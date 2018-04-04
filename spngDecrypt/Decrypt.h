#pragma once

#include <array>
#include <string>
#include <vector>
#include "Tools.h"

/**
* 解密PNG圖片
* @param filelist 文件列表
*/
void DecryptPNG(const std::vector<std::wstring> &filelist, const aes_key &key);