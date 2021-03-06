#include "stdafx.h"
#include "Decrypt.h"
#include "CRC32.h"
#include "Files.h"
#include <fstream>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 解密PNG圖片
void DecryptPNG(const vector<wstring> &filelist, const aes_key &key)
{
    for (auto &filename : filelist)
    {
        ifstream in_file(filename, ios::binary | ios::ate);
        if (!in_file.is_open())
        {
            wcout << L"打開" << filename << L" 失敗！\n";
            return;
        }

        // 讀取數據塊位置
        uint32_t end_pos = (uint32_t)in_file.tellg();
        in_file.seekg(end_pos - sizeof(uint32_t));
        uint32_t block_start_pos = ntohl(*reinterpret_cast<uint32_t *>(&(ReadSome<sizeof(uint32_t)>(in_file)[0])));
        in_file.seekg(block_start_pos);

        // 獲取數據塊大小
        const uint32_t block_size = ntohl(*reinterpret_cast<uint32_t *>(&ReadSome<sizeof(uint32_t)>(in_file)[0]));

        // 解密數據塊信息
        auto block_info = ReadLarge(in_file, uint32_t(end_pos - block_start_pos - sizeof(uint32_t) * 2));
        DecryptBlock(block_info, key);

        // 驗證校驗和
        block_info.seekg(block_size);
        uint32_t crc32 = ntohl(*reinterpret_cast<uint32_t *>(&ReadSome<sizeof(uint32_t)>(block_info)[0]));
        if (crc32 != CRC32(block_info.str().substr(0, block_size)))
        {
            wcout << L"校驗和驗證失敗！\n";
            continue;
        }

        // 創建PNG文件
        ofstream out_file(path::splitext(filename)[0] + L".png", ios::binary);
        if (!out_file.is_open())
        {
            wcout << L"創建" << path::splitext(filename)[1] << L".png" << L" 失敗！\n";
            continue;
        }

        // 寫入文件頭
        SteamCopy(out_file, HEAD_DATA, sizeof(HEAD_DATA));

        // 讀取數據塊
        block_info.seekg(0);
        uint64_t read_size = 0;
        while (true)
        {
            // 驗證數據有效性
            if (block_info.tellg() >= block_size)
            {
                out_file.clear();
                wcout << L"the %s file format error!\n";
                break;
            }

            // 讀取數據塊信息
            Block block;
            memcpy(&block, &ReadSome<sizeof(Block)>(block_info)[0], sizeof(Block));

            // 寫入數據塊長度
            SteamCopy(out_file, &block.size, sizeof(block.size));

            // 大小端轉換
            block.pos = ntohl(block.pos);
            block.size = ntohl(block.size);

            // 寫入數據塊名稱
            SteamCopy(out_file, &block.name, sizeof(block.name));

            // 寫入數據塊內容
            string s_name(block.name, sizeof(block.name));
            if (strcmp(s_name.c_str(), "IHDR") == 0)
            {
                IHDRBlock ihdr;
                memcpy(&ihdr, &block, sizeof(Block));
                memcpy(((char *)&ihdr) + sizeof(Block), &ReadSome<sizeof(IHDRBlock) - sizeof(Block)>(block_info)[0], sizeof(IHDRBlock) - sizeof(Block));
                SteamCopy(out_file, ihdr.data, sizeof(ihdr.data));
            }
            else if (strcmp(s_name.c_str(), "IEND") == 0)
            {
                SteamCopy(out_file, IEND_DATA, sizeof(IEND_DATA));
                wcout << L"成功解密：" << filename << L"\n";
                break;
            }
            else
            {
                in_file.seekg(read_size);
                StreamMove(out_file, in_file, block.size + CRC_SIZE);
                read_size += block.size + CRC_SIZE;
            }
        }
    }
}