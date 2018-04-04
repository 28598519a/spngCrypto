#pragma once

#include <array>
#include <string>
#include <vector>
#include "Tools.h"

/**
* 加密PNG圖片
* @param filelist 文件劉表
*/
void EncryptPNG(const std::vector<std::wstring> &filelist, const aes_key &key);
