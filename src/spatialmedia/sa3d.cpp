/*****************************************************************************
 * 
 * Copyright 2016 Varol Okan. All rights reserved.
 * Copyright (c) 2024 Meltytech, LLC
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the governing permissions and
 * limitations under the License.
 * 
 ****************************************************************************/

/* 【文件说明】：MPEG SA3D原子处理类
 * 【功能】：支持SA3D MPEG-4空间音频盒的注入和处理
 * 【规范】：基于Google空间媒体RFC规范
 * 【参考】：https://github.com/google/spatial-media/docs/spatial-audio-rfc.md
 */

#include <cmath>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string.h>

#include "constants.h"
#include "sa3d.h"

// 【构造函数】：初始化SA3D原子
// 【功能】：设置SA3D原子的默认参数和枚举值映射
SA3DBox::SA3DBox ()
  : Box ( )  // 调用基类构造函数
{
  memcpy ( m_name, constants::TAG_SA3D, 4 );  // 设置原子名称为"SA3D"
  m_iHeaderSize    = 8;       // 标准头部大小8字节
  m_iPosition      = 0;       // 文件位置初始化为0
  m_iContentSize   = 0;       // 内容大小初始为0
  
  // Ambisonic音频参数初始化
  m_iVersion       = 0;       // 版本号（当前为0）
  m_iAmbisonicType = 0;       // Ambisonic类型（0=periphonic）
  m_iAmbisonicOrder= 0;       // Ambisonic阶数（1阶、2阶等）
  m_iAmbisonicChannelOrdering = 0;  // 通道排序（0=ACN）
  m_iAmbisonicNormalization   = 0;  // 归一化方法（0=SN3D）
  m_iNumChannels   = 0;       // 通道数量

  // 枚举值映射表初始化
  m_AmbisonicTypes["periphonic"]    = 0;  // 环绕式Ambisonic
  m_AmbisonicOrderings["ACN"]       = 0;  // Ambisonic通道编号（标准）
  m_AmbisonicNormalizations["SN3D"] = 0;  // Schmidt半归一化（标准）
}

// 【析构函数】
SA3DBox::~SA3DBox ( )
{
  // 自动清理m_ChannelMap等STL容器
}

// 【核心功能】：从文件流加载SA3D原子
// 【说明】：解析MP4文件中的SA3D空间音频元数据原子
Box *SA3DBox::load ( std::fstream &fs, uint32_t iPos, uint32_t iEnd )
{
  SA3DBox *pNewBox = NULL;
  
  // 位置处理：如果传入位置为0，使用当前文件位置
  if ( iPos < 0 )
       iPos = fs.tellg ( );

  char name[4];

  // 读取原子头部信息
  fs.seekg( iPos );
  uint32_t iSize = readUint32 ( fs );  // 读取原子大小
  fs.read ( name, 4 );                 // 读取原子名称
  
  // 处理扩展大小格式（当size=1时）
  if ( iSize == 1 )  { 
    iSize = (int32_t)readUint64 ( fs );  // 读取64位实际大小
  }

  // 验证原子类型是否为SA3D
  if ( 0 != memcmp ( name, constants::TAG_SA3D, sizeof ( *constants::TAG_SA3D ) ) )  {
    std::cerr << "Error: box is not an SA3D box." << std::endl;
    return NULL;
  }

  // 边界检查
  if ( iPos + iSize > iEnd )  {
    std::cerr << "Error: SA3D box size exceeds bounds." << std::endl;
    return NULL;
  }

  // 创建新的SA3D原子对象
  pNewBox = new SA3DBox ( );
  pNewBox->m_iPosition = iPos;
  pNewBox->m_iContentSize    = iSize - pNewBox->m_iHeaderSize;  // 计算内容大小
  
  // 读取SA3D特有的音频元数据字段
  pNewBox->m_iVersion        = readUint8  ( fs );  // 版本号（1字节）
  pNewBox->m_iAmbisonicType  = readUint8  ( fs );  // Ambisonic类型（1字节）
  pNewBox->m_iAmbisonicOrder = readUint32 ( fs );  // Ambisonic阶数（4字节）
  pNewBox->m_iAmbisonicChannelOrdering = readUint8 ( fs );  // 通道排序（1字节）
  pNewBox->m_iAmbisonicNormalization   = readUint8 ( fs );  // 归一化方法（1字节）
  pNewBox->m_iNumChannels    = readUint32 ( fs );  // 通道数量（4字节）

  // 读取通道映射表
  for ( auto i = 0U; i< pNewBox->m_iNumChannels; i++ )  {
    uint32_t iVal = readUint32 ( fs );  // 每个通道映射值（4字节）
    pNewBox->m_ChannelMap.push_back ( iVal );
  }
  
  return pNewBox;
}

// 【功能】：创建新的SA3D原子
// 【参数】：iNumChannels - 音频通道数量
// 【说明】：根据通道数量自动计算Ambisonic阶数
Box *SA3DBox::create (int32_t iNumChannels)
{
  SA3DBox *pNewBox = new SA3DBox ( );
  pNewBox->m_iHeaderSize   = 8;
  memcpy ( pNewBox->m_name, constants::TAG_SA3D, 4 );
  
  // 计算Ambisonic阶数：阶数 = sqrt(通道数) - 1
  // 例如：4通道 → 1阶，9通道 → 2阶，16通道 → 3阶
  pNewBox->m_iAmbisonicOrder = ::sqrt(iNumChannels) - 1;

  // 设置版本和内容大小计算
  pNewBox->m_iVersion       = 0; // uint8版本号
  pNewBox->m_iContentSize += 1;  // 版本字段大小
  
  pNewBox->m_iContentSize += 1;  // Ambisonic类型字段大小
  pNewBox->m_iContentSize += 4;  // Ambisonic阶数字段大小
  pNewBox->m_iContentSize += 1;  // 通道排序字段大小
  pNewBox->m_iContentSize += 1;  // 归一化方法字段大小
  
  pNewBox->m_iNumChannels = iNumChannels;
  pNewBox->m_iContentSize += 4;  // 通道数字段大小

  // 初始化通道映射表（线性映射：0,1,2,3,...）
  for (uint32_t i = 0; i < iNumChannels; i++) {
    pNewBox->m_ChannelMap.push_back(i);  // 默认顺序映射
    pNewBox->m_iContentSize += 4;        // 每个映射值4字节
  }
  
  return pNewBox;
}

// 【功能】：保存SA3D原子到输出文件流
// 【说明】：将SA3D音频元数据写入MP4文件
void SA3DBox::save (std::fstream &fsIn, std::fstream &fsOut , int32_t)
{
  (void) fsIn; // 未使用参数（SA3D原子内容完全在内存中生成）
 
  // 写入原子头部
  if ( m_iHeaderSize == 16 )  {
    // 扩展大小格式
    writeUint32 ( fsOut, 1 );        // 写入扩展标记
    fsOut.write ( m_name,  4 );      // 写入原子名称
    writeUint64 ( fsOut, size() );   // 写入扩展大小
  }
  else if ( m_iHeaderSize == 8 )  {
    // 标准大小格式
    writeUint32 ( fsOut, size() );   // 写入标准大小
    fsOut.write ( m_name, 4 );       // 写入原子名称
  }

  // 写入SA3D特有的音频元数据字段
  writeUint8  ( fsOut, m_iVersion );                      // 版本号
  writeUint8  ( fsOut, m_iAmbisonicType );                // Ambisonic类型
  writeUint32 ( fsOut, m_iAmbisonicOrder );               // Ambisonic阶数
  writeUint8  ( fsOut, m_iAmbisonicChannelOrdering );     // 通道排序
  writeUint8  ( fsOut, m_iAmbisonicNormalization );       // 归一化方法
  writeUint32 ( fsOut, m_iNumChannels );                  // 通道数量

  // 写入通道映射表
  std::vector<uint32_t>::iterator it = m_ChannelMap.begin ( );
  while ( it != m_ChannelMap.end ( ) )  {
    writeUint32 ( fsOut, *it++ );    // 写入每个通道映射值
  }
}

// 【功能】：获取Ambisonic类型名称
// 【说明】：根据枚举值返回对应的类型字符串
const char *SA3DBox::ambisonic_type_name ( )
{
  // 待实现：根据m_iAmbisonicType返回对应的字符串
  // 例如：0 → "periphonic"
  return NULL;
}

// 【功能】：获取通道排序名称
const char *SA3DBox::ambisonic_channel_ordering_name ( )
{
  // 待实现：根据m_iAmbisonicChannelOrdering返回对应的字符串
  // 例如：0 → "ACN"
  return NULL;
}

// 【功能】：获取归一化方法名称
const char * SA3DBox::ambisonic_normalization_name ( )
{
  // 待实现：根据m_iAmbisonicNormalization返回对应的字符串
  // 例如：0 → "SN3D"
  return NULL;
}

// 【功能】：将通道映射表转换为字符串
// 【说明】：用于调试和显示目的
std::string SA3DBox::mapToString ( )
{
  std::stringstream ss;
  // 使用流迭代器将vector转换为逗号分隔的字符串
  std::copy ( m_ChannelMap.begin ( ), m_ChannelMap.end ( ), 
              std::ostream_iterator<uint32_t>(ss, ", " ) );
  return ss.str ( );
}

// 【功能】：打印SA3D原子内容到控制台
// 【说明】：用于调试和验证SA3D元数据
void SA3DBox::print_box ( )
{
  const char *ambisonic_type   = ambisonic_type_name ( );
  const char *channel_ordering = ambisonic_channel_ordering_name ( );
  const char *ambisonic_normalization = ambisonic_normalization_name ( );
  std::string str = mapToString ( );

  std::cout << "\t\tAmbisonic Type: " << ambisonic_type << std::endl;
  std::cout << "\t\tAmbisonic Order: " << m_iAmbisonicOrder << std::endl;
  std::cout << "\t\tAmbisonic Channel Ordering: " << channel_ordering << std::endl;
  std::cout << "\t\tAmbisonic Normalization: " << ambisonic_normalization << std::endl;
  std::cout << "\t\tNumber of Channels: " << m_iNumChannels << std::endl;
  std::cout << "\t\tChannel Map: " << str << std::endl;
}

// 【功能】：生成简洁的音频元数据字符串
// 【说明】：单行格式，便于日志记录和显示
std::string SA3DBox::get_metadata_string ( )
{
  std::ostringstream str;
  str << ambisonic_normalization_name ( ) << ", ";     // 归一化方法
  str << ambisonic_channel_ordering_name ( ) << ", ";  // 通道排序
  str << ambisonic_type_name ( ) << ", Order ";        // 类型
  str << m_iAmbisonicOrder << ", ";                    // 阶数
  str << m_iNumChannels << " Channel(s), Channel Map: "; // 通道数
  str << mapToString ( );                              // 通道映射

  return str.str ( );
SA3DBox::SA3DBox()
    : Box()
{
    memcpy(m_name, constants::TAG_SA3D, 4);
    m_iHeaderSize = 8;
    m_iPosition = 0;
    m_iContentSize = 0;
    m_iVersion = 0;
    m_iAmbisonicType = 0;
    m_iAmbisonicOrder = 0;
    m_iAmbisonicChannelOrdering = 0;
    m_iAmbisonicNormalization = 0;
    m_iNumChannels = 0;

    m_AmbisonicTypes["periphonic"] = 0;
    m_AmbisonicOrderings["ACN"] = 0;
    m_AmbisonicNormalizations["SN3D"] = 0;
}

SA3DBox::~SA3DBox() {}

// Loads the SA3D box located at position pos in a mp4 file.
Box *SA3DBox::load(std::fstream &fs, uint32_t iPos, uint32_t iEnd)
{
    SA3DBox *pNewBox = NULL;
    if (iPos < 0)
        iPos = fs.tellg();

    char name[4];

    fs.seekg(iPos);
    uint32_t iSize = readUint32(fs);
    fs.read(name, 4);
    // Test if iSize == 1
    // Added for 360Tube to have load and save in-sync.
    if (iSize == 1) {
        iSize = (int32_t) readUint64(fs);
    }

    if (0 != memcmp(name, constants::TAG_SA3D, sizeof(*constants::TAG_SA3D))) {
        std::cerr << "Error: box is not an SA3D box." << std::endl;
        return NULL;
    }

    if (iPos + iSize > iEnd) {
        std::cerr << "Error: SA3D box size exceeds bounds." << std::endl;
        return NULL;
    }

    pNewBox = new SA3DBox();
    pNewBox->m_iPosition = iPos;
    pNewBox->m_iContentSize = iSize - pNewBox->m_iHeaderSize;
    pNewBox->m_iVersion = readUint8(fs);
    pNewBox->m_iAmbisonicType = readUint8(fs);
    pNewBox->m_iAmbisonicOrder = readUint32(fs);
    pNewBox->m_iAmbisonicChannelOrdering = readUint8(fs);
    pNewBox->m_iAmbisonicNormalization = readUint8(fs);
    pNewBox->m_iNumChannels = readUint32(fs);

    for (auto i = 0U; i < pNewBox->m_iNumChannels; i++) {
        uint32_t iVal = readUint32(fs);
        pNewBox->m_ChannelMap.push_back(iVal);
    }
    return pNewBox;
}

Box *SA3DBox::create(int32_t iNumChannels)
{
    // audio_metadata: dictionary ('ambisonic_type': string, 'ambisonic_order': int),

    SA3DBox *pNewBox = new SA3DBox();
    pNewBox->m_iHeaderSize = 8;
    memcpy(pNewBox->m_name, constants::TAG_SA3D, 4);
    pNewBox->m_iAmbisonicOrder = ::sqrt(iNumChannels) - 1;

    pNewBox->m_iVersion = 0;      // # uint8
    pNewBox->m_iContentSize += 1; // # uint8
    //  pNewBox->m_iAmbisonicType= pNewBox->m_AmbisonicTypes[amData["ambisonic_type"]];
    pNewBox->m_iContentSize += 1; // # uint8
                                  //  pNewBox->m_iAmbisonicOrder = amData["ambisonic_order"];
    pNewBox->m_iContentSize += 4; // # uint32
    //  pNewBox->m_iAmbisonicChannelOrdering = pNewBox->m_AmbisonicOrderings[ amData["ambisonic_channel_ordering"]]
    pNewBox->m_iContentSize += 1; // # uint8
    //  pNewBox->m_iAmbisonicNormalization = pNewBox->m_AmbisonicNormalizations[ amData["ambisonic_normalization"]]
    pNewBox->m_iContentSize += 1; // # uint8
    pNewBox->m_iNumChannels = iNumChannels;
    pNewBox->m_iContentSize += 4; // # uint32

    // std::vector<int> map; // = amData["channel_map"];
    // std::vector<int>::iterator it = map.begin ( );
    // while ( it != map.end ( ) )  {
    //   pNewBox->m_ChannelMap.push_back ( *it++ );
    //   pNewBox->m_iContentSize += 4;
    // }
    for (uint32_t i = 0; i < iNumChannels; i++) {
        pNewBox->m_ChannelMap.push_back(i);
        pNewBox->m_iContentSize += 4; // # uint32
    }
    return pNewBox;
}

void SA3DBox::save(std::fstream &fsIn, std::fstream &fsOut, int32_t)
{
    (void) fsIn; // unused
    //char tmp, name[4];

    if (m_iHeaderSize == 16) {
        writeUint32(fsOut, 1);
        fsOut.write(m_name, 4);
        writeUint64(fsOut, size());
        //fsOut.write ( name,  4 ); I think this is a bug in the original code here.
    } else if (m_iHeaderSize == 8) {
        writeUint32(fsOut, size());
        fsOut.write(m_name, 4);
    }

    writeUint8(fsOut, m_iVersion);
    writeUint8(fsOut, m_iAmbisonicType);
    writeUint32(fsOut, m_iAmbisonicOrder);
    writeUint8(fsOut, m_iAmbisonicChannelOrdering);
    writeUint8(fsOut, m_iAmbisonicNormalization);
    writeUint32(fsOut, m_iNumChannels);

    std::vector<uint32_t>::iterator it = m_ChannelMap.begin();
    while (it != m_ChannelMap.end()) {
        writeUint32(fsOut, *it++);
    }
}

const char *SA3DBox::ambisonic_type_name()
{
    //return  (key for key,value in SA3DBox.ambisonic_types.items()
    //  if value==self.ambisonic_type).next()
    return NULL;
}

const char *SA3DBox::ambisonic_channel_ordering_name()
{
    //return (key for key,value in SA3DBox.ambisonic_orderings.items()
    //  if value==self.ambisonic_channel_ordering).next()
    return NULL;
}

const char *SA3DBox::ambisonic_normalization_name()
{
    //return (key for key,value in SA3DBox.ambisonic_normalizations.items()
    //  if value==self.ambisonic_normalization).next()
    return NULL;
}

std::string SA3DBox::mapToString()
{
    std::stringstream ss;
    std::copy(m_ChannelMap.begin(), m_ChannelMap.end(), std::ostream_iterator<uint32_t>(ss, ", "));
    return ss.str();
}

void SA3DBox::print_box()
{
    // Prints the contents of this spatial audio (SA3D) box to the console.
    const char *ambisonic_type = ambisonic_type_name();
    const char *channel_ordering = ambisonic_channel_ordering_name();
    const char *ambisonic_normalization = ambisonic_normalization_name();
    std::string str = mapToString();

    std::cout << "\t\tAmbisonic Type: " << ambisonic_type << std::endl;
    std::cout << "\t\tAmbisonic Order: " << m_iAmbisonicOrder << std::endl;
    std::cout << "\t\tAmbisonic Channel Ordering: " << channel_ordering << std::endl;
    std::cout << "\t\tAmbisonic Normalization: " << ambisonic_normalization << std::endl;
    std::cout << "\t\tNumber of Channels: " << m_iNumChannels << std::endl;
    std::cout << "\t\tChannel Map: %s" << str << std::endl;
}

std::string SA3DBox::get_metadata_string()
{
    // Outputs a concise single line audio metadata string.
    std::ostringstream str;
    str << ambisonic_normalization_name() << ", ";
    str << ambisonic_channel_ordering_name() << ", ";
    str << ambisonic_type_name() << ", Order ";
    str << m_iAmbisonicOrder << ", ";
    str << m_iNumChannels << ", Channel(s), Channel Map: ";
    str << mapToString();

    return str.str();
}
