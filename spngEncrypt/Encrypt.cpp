#include "stdafx.h"
#include "Encrypt.h"
#include "CRC32.h"
#include "Files.h"
#include <fstream>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

/**
* 寫入單個文件數據
* @param filename 文件名稱
* @param outstream 輸出文件流
* @return 數據塊信息
*/
stringstream WriteFileData(const wstring &filename, ofstream &outstream)
{
    ifstream file;
    stringstream block_info;
    file.open(filename, ios::binary);
    if (!file.is_open())
    {
        wcout << L"打開" << filename << L" 失敗！\n";
        return block_info;
    }

    // 讀取文件頭
    auto head = ReadSome<HEAD_SIZE>(file);

    // 讀取數據塊
    while (true)
    {
        Block block;

        // 獲取數據塊長度
        auto lenght = ReadSome<4>(file);
        if (file.eof()) break;
        auto block_size = ntohl(*reinterpret_cast<uint32_t *>(&lenght[0]));

        // 獲取數據塊名稱
        auto block_name = &(ReadSome<4>(file)[0]);

        // 獲取數據塊內容
        auto block_data = ReadLarge(file, block_size + CRC_SIZE);

        // 數據塊信息
        block.size = htonl(block_size);
        block.pos = htonl((uint32_t)outstream.tellp());
        memcpy(block.name, &block_name[0], sizeof(block.name));

        // 根據數據類型進行處理
        string s_name(block.name, sizeof(block.name));
        if (strcmp(s_name.c_str(), "IHDR") == 0)
        {
            IHDRBlock ihdr;
            ihdr.block = block;
            memcpy(ihdr.data, block_data.str().c_str(), sizeof(ihdr.data));
            SteamCopy(block_info, &ihdr, sizeof(IHDRBlock));
        }
        else if (strcmp(s_name.c_str(), "IEND") == 0)
        {
            SteamCopy(block_info, &block, sizeof(Block));
        }
        else
        {
            SteamCopy(block_info, &block, sizeof(Block));
            StreamMove(outstream, block_data, block_size + CRC_SIZE);
        }
    }
    return block_info;
}

// 加密PNG圖片
void EncryptPNG(const vector<wstring> &filelist, const aes_key &key)
{
    for (auto &filename : filelist)
    {
        // 寫入文件數據
        wstring out_path = path::splitext(filename)[0] + L".spng";
        ofstream out_file(out_path, ios::binary);
        if (!out_file.is_open())
        {
            wcout << L"創建" << filename << L" 失敗！\n";
            continue;
        }

        // 寫入文件數據
        stringstream block_info = WriteFileData(filename, out_file);
        uint32_t block_size = uint32_t(block_info.tellp() - block_info.tellg());
        if (block_size == 0) continue;

        // 記錄起始位置
        uint32_t block_start_pos = htonl((uint32_t)out_file.tellp());

        // 寫入數據塊信息大小
        block_size = htonl(block_size);
        SteamCopy(out_file, &block_size, sizeof(block_size));

        // 寫入校驗和
        uint32_t crc32 = htonl(CRC32(block_info.str()).GetChecksum());
        SteamCopy(block_info, &crc32, sizeof(crc32));

        // 數據塊信息加密
        EncryptBlock(block_info, key);

        // 寫入數據塊信息
        StreamMove(out_file, block_info, uint32_t(block_info.tellp() - block_info.tellg()));

        // 寫入數據塊信息位置
        char *user_data = reinterpret_cast<char *>(&block_start_pos);
        for (unsigned int i = 0; i < sizeof(block_start_pos); ++i) out_file.put(user_data[i]);

        wcout << L"已生成：" << out_path << L"\n";
    }
}