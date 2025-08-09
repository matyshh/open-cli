#ifndef OPENCLI_DOWNLOAD_UTILS_H
#define OPENCLI_DOWNLOAD_UTILS_H

#include <stdbool.h>

bool download_file(const char *url, const char *dest_path);
bool extract_zip(const char *zip_path, const char *dest_dir);
bool extract_tgz(const char *tgz_path, const char *dest_dir);

#endif 