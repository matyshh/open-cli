#include "download_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#else
#include <curl/curl.h>
#endif

bool download_file(const char *url, const char *dest_path) {
    
#ifdef _WIN32
    // Windows implementation using WinInet
    HINTERNET hInternet, hUrl;
    HANDLE hFile;
    DWORD bytesRead, bytesWritten;
    char buffer[4096];
    bool result = false;

    hInternet = InternetOpen("opencli/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        DWORD error = GetLastError();
        fprintf(stderr, "Failed to initialize WinInet (error code: %lu)\n", error);
        return false;
    }

    hUrl = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        DWORD error = GetLastError();
        fprintf(stderr, "Failed to open URL (error code: %lu)\n", error);
        
        // Check common error codes
        if (error == ERROR_INTERNET_NAME_NOT_RESOLVED) {
            fprintf(stderr, "DNS name not resolved. Check your internet connection.\n");
        } else if (error == ERROR_INTERNET_CANNOT_CONNECT) {
            fprintf(stderr, "Cannot connect to server.\n");
        } else if (error == ERROR_INTERNET_CONNECTION_ABORTED) {
            fprintf(stderr, "Connection aborted.\n");
        } else if (error == ERROR_INTERNET_CONNECTION_RESET) {
            fprintf(stderr, "Connection reset.\n");
        } else if (error == ERROR_HTTP_INVALID_SERVER_RESPONSE) {
            fprintf(stderr, "Invalid server response.\n");
        }
        
        InternetCloseHandle(hInternet);
        return false;
    }

    // Check HTTP status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfo(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL)) {
        if (statusCode != 200) {
            fprintf(stderr, "HTTP error: %lu\n", statusCode);
            if (statusCode == 404) {
                fprintf(stderr, "File not found on server (HTTP 404)\n");
            } else if (statusCode == 403) {
                fprintf(stderr, "Access forbidden (HTTP 403)\n");
            }
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return false;
        }
    }

    hFile = CreateFile(dest_path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        fprintf(stderr, "Failed to create file (error code: %lu)\n", error);
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Download the file
    DWORD totalBytes = 0;
    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || bytesRead != bytesWritten) {
            fprintf(stderr, "Failed to write to file\n");
            CloseHandle(hFile);
            InternetCloseHandle(hUrl);
            InternetCloseHandle(hInternet);
            return false;
        }
        totalBytes += bytesRead;
    }

    printf("Download completed. %lu bytes transferred.\n", totalBytes);
    
    result = true;
    CloseHandle(hFile);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return result;
#else
    // Linux/macOS implementation using libcurl
    CURL *curl;
    FILE *fp;
    CURLcode res;
    bool result = false;

    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return false;
    }

    fp = fopen(dest_path, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to create file: %s\n", dest_path);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "opencli/1.0");

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "Failed to download file: %s\n", curl_easy_strerror(res));
        fclose(fp);
        curl_easy_cleanup(curl);
        return false;
    }

    // Check HTTP status code
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        fprintf(stderr, "HTTP error: %ld\n", http_code);
        if (http_code == 404) {
            fprintf(stderr, "File not found on server (HTTP 404)\n");
        } else if (http_code == 403) {
            fprintf(stderr, "Access forbidden (HTTP 403)\n");
        }
        fclose(fp);
        curl_easy_cleanup(curl);
        return false;
    }

    fclose(fp);
    curl_easy_cleanup(curl);
    return true;
#endif
}

bool extract_zip(const char *zip_path, const char *dest_dir) {
    printf("Extracting %s...\n", zip_path);
    
#ifdef _WIN32
    // Try multiple extraction methods on Windows
    int result = -1;
    char cmd[4096];
    
    // Method 1: Try using Windows COM objects (works on all Windows versions)
    sprintf_s(cmd, sizeof(cmd), 
        "powershell -Command \"& { try { "
        "$shell = New-Object -ComObject Shell.Application; "
        "$zip = $shell.NameSpace('%s'); "
        "if ($zip -eq $null) { Write-Error 'Failed to open ZIP file'; exit 1 } "
        "foreach ($item in $zip.Items()) { "
        "  $shell.NameSpace('%s').CopyHere($item, 0x14); "
        "} "
        "exit 0 "
        "} catch { Write-Host $_.Exception.Message; exit 1 } }\"", 
        zip_path, dest_dir);
        
    result = system(cmd);
    
    if (result == 0) {
        return true;
    }
    
    // Method 2: Try using 7-Zip if available
    sprintf_s(cmd, sizeof(cmd), "where 7z > nul 2>&1 && 7z x -y -o\"%s\" \"%s\"", dest_dir, zip_path);
    result = system(cmd);
    
    if (result == 0) {
        return true;
    }
    
    // Method 3: Try older PowerShell method (should work on most versions)
    sprintf_s(cmd, sizeof(cmd), 
        "powershell -Command \"& { try { "
        "$ErrorActionPreference = 'Stop'; "
        "Add-Type -AssemblyName System.IO.Compression.FileSystem; "
        "[System.IO.Compression.ZipFile]::ExtractToDirectory('%s', '%s'); "
        "exit 0 "
        "} catch { "
        "Write-Host $_.Exception.Message; "
        "exit 1 "
        "} }\"", 
        zip_path, dest_dir);
        
    result = system(cmd);
    
    if (result == 0) {
        return true;
    }
    
    fprintf(stderr, "Failed to extract %s\n", zip_path);
    return false;
#else
    // Linux/macOS implementation
    char cmd[1024];
    sprintf(cmd, "unzip -o '%s' -d '%s'", zip_path, dest_dir);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Extraction failed with exit code: %d\n", result);
        return false;
    }
    return true;
#endif
}

bool extract_tgz(const char *tgz_path, const char *dest_dir) {
    printf("Extracting %s...\n", tgz_path);
#ifdef _WIN32
    char cmd[1024];
    sprintf_s(cmd, sizeof(cmd), "powershell -Command \"tar -xzf '%s' -C '%s'\"", tgz_path, dest_dir);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Extraction failed with exit code: %d\n", result);
        return false;
    }
    return true;
#else
    // Linux/macOS implementation
    char cmd[1024];
    sprintf(cmd, "tar -xzf '%s' -C '%s'", tgz_path, dest_dir);
    int result = system(cmd);
    if (result != 0) {
        fprintf(stderr, "Extraction failed with exit code: %d\n", result);
        return false;
    }
    return true;
#endif
} 