/*****************************************************************************
 * 
 * Copyright 2016 Varol Okan. All rights reserved.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the governing permissions and
 * limitations under the License.
 * 
 ****************************************************************************/

#include <iostream>
#include <stdlib.h>
#include <string.h>

#include "mpeg4_container.h"

// 【构造函数】：初始化MP4容器对象
// 【功能】：设置MP4文件的关键原子指针和初始状态
Mpeg4Container::Mpeg4Container ( )
  : Container ( )  // 调用基类构造函数
{
  m_pMoovBox      = NULL;      // 电影元数据原子指针（必须存在）
  m_pFreeBox      = NULL;      // 空闲空间原子指针（可选）
  m_pFirstMDatBox = NULL;      // 第一个媒体数据原子指针（必须存在）
  m_pFTYPBox      = NULL;      // 文件类型原子指针（通常存在）
  m_iFirstMDatPos = 0;         // 第一个媒体数据的起始位置
}

// 【析构函数】
Mpeg4Container::~Mpeg4Container ( )
{
  // 注意：基类析构函数会自动清理m_listContents中的原子
}

// 【核心功能】：从文件流加载MP4容器
// 【说明】：这是MP4文件解析的入口点，加载整个文件结构
Mpeg4Container *Mpeg4Container::load ( std::fstream &fsIn )
{
  // 获取文件总大小
  fsIn.seekg ( 0, std::ios::end );     // 定位到文件末尾
  int32_t iSize = fsIn.tellg ( );      // 获取文件大小
  fsIn.seekg ( 0, std::ios::beg );     // 回到文件开头
  
  // 加载文件中的所有原子
  std::vector<Box *> list = load_multiple ( fsIn, 0, iSize );

  // 检查是否成功加载了原子
  if ( list.empty ( ) )  {
    std::cerr << "Error, failed to load .mp4 file." << std::endl;
    return NULL; 
  }
  
  // 创建新的MP4容器对象
  Mpeg4Container *pNewBox = new Mpeg4Container ( );
  pNewBox->m_listContents = list;

  // 识别和设置关键原子指针
  std::vector<Box *>::iterator it = list.begin ( );
  while ( it != list.end ( ) )  {
    Box *pBox = *it++;
    
    // 识别电影元数据原子（moov）
    if ( memcmp ( pBox->m_name, "moov", 4 ) == 0 )
      pNewBox->m_pMoovBox = pBox;
    
    // 识别空闲空间原子（free）
    if ( memcmp ( pBox->m_name, "free", 4 ) == 0 )
      pNewBox->m_pFreeBox = pBox;
    
    // 识别第一个媒体数据原子（mdat）
    if ( ( memcmp ( pBox->m_name, "mdat", 4 ) == 0 ) &&
         ( ! pNewBox->m_pFirstMDatBox ) )  // 只记录第一个mdat原子
      pNewBox->m_pFirstMDatBox = pBox;
    
    // 识别文件类型原子（ftyp）
    if ( memcmp ( pBox->m_name, "ftyp", 4 ) == 0 )
      pNewBox->m_pFTYPBox = pBox;
  }
  
  // 验证必需原子的存在
  if ( ! pNewBox->m_pMoovBox )  {
    std::cerr << "Error, file does not contain moov box." << std::endl;
    delete pNewBox;
    return NULL;
  }
  if ( ! pNewBox->m_pFirstMDatBox )  {
    std::cerr << "Error, file does not contain mdat box." << std::endl;
    delete pNewBox;
    return NULL;
  }
  
  // 计算媒体数据的实际起始位置（跳过头部）
  pNewBox->m_iFirstMDatPos  = pNewBox->m_pFirstMDatBox->m_iPosition;
  pNewBox->m_iFirstMDatPos += pNewBox->m_pFirstMDatBox->m_iHeaderSize;
  
  // 计算容器的总内容大小
  pNewBox->m_iContentSize   = 0;
  it = pNewBox->m_listContents.begin ( );
  while ( it != pNewBox->m_listContents.end ( ) )  {
    Box *pBox = *it++;
    pNewBox->m_iContentSize += pBox->size ( );  // 累加所有原子大小
  }
  
  return pNewBox;
}

// 【功能】：合并操作（MP4容器不支持合并）
// 【说明】：MP4文件格式不允许简单的容器合并，需要复杂的重映射
void Mpeg4Container::merge ( Box *pElement )
{
  (void) pElement; // 未使用参数
  
  // MP4容器不支持合并操作
  // 原因：合并MP4文件需要重新计算所有媒体数据的索引和偏移量
  std::cerr << "Cannot merge mpeg4 files" << std::endl;
  exit ( 0 );  // 直接退出程序，因为这是不可恢复的错误
}

// 【功能】：打印MP4文件结构
// 【说明】：以树形结构显示MP4文件的完整原子层次
void Mpeg4Container::print_structure ( const char *pIndent )
{
  // 打印MP4容器的总体信息
  std::cout << "mpeg4 [" << m_iContentSize << "]" << std::endl;
  
  // 递归打印所有子原子的结构
  uint32_t iCount = m_listContents.size ( );
  std::string strIndent = pIndent;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++;
    
    // 设置树形显示的前缀
    strIndent = " ├──";        // 中间节点连接符
    if ( --iCount <= 0 )
      strIndent = " └──";      // 最后一个节点连接符
      
    pBox->print_structure ( strIndent.c_str ( ) );  // 递归打印子原子
  }
}

// 【核心功能】：保存MP4容器到文件
// 【说明】：处理媒体数据偏移量更新并保存整个文件结构
void Mpeg4Container::save ( std::fstream &fsIn, std::fstream &fsOut, int32_t )
{
  // 步骤1：重新计算容器大小（递归更新所有子原子）
  resize ( );
  
  // 步骤2：计算新的媒体数据偏移量
  // 原因：文件结构改变可能导致媒体数据位置变化，需要更新索引
  uint32_t iNewPos = 0;
  std::vector<Box *>::iterator it = m_listContents.begin ( );
  
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++;
    
    // 找到第一个媒体数据原子（mdat）的位置
    if ( memcmp( pBox->m_name, constants::TAG_MDAT, 4 ) == 0 )  {
      iNewPos += pBox->m_iHeaderSize;  // 只计算头部，内容位置在头部之后
      break;
    }
    iNewPos += pBox->size ( );  // 累加前面所有原子的大小
  }
  
  // 计算偏移量变化（新旧媒体数据位置差异）
  uint32_t iDelta = iNewPos - m_iFirstMDatPos;
  
  // 步骤3：递归保存所有原子，传递偏移量用于更新索引
  it = m_listContents.begin ( );
  while ( it != m_listContents.end () )  {
    Box *pBox = *it++; 
    pBox->save ( fsIn, fsOut, iDelta );  // 递归保存，自动处理索引更新
  }
Mpeg4Container::Mpeg4Container()
    : Container()
{
    m_pMoovBox = NULL;
    m_pFreeBox = NULL;
    m_pFirstMDatBox = NULL;
    m_pFTYPBox = NULL;
    m_iFirstMDatPos = 0;
}

Mpeg4Container::~Mpeg4Container() {}

Mpeg4Container *Mpeg4Container::load(std::fstream &fsIn) //, uint32_t /* iPos */, uint32_t /* iEnd */ )
{
    // Load the mpeg4 file structure of a file.
    //  fsIn.seekg ( 0, 2 );
    int32_t iSize = fsIn.tellg();
    std::vector<Box *> list = load_multiple(fsIn, 0, iSize);

    if (list.empty()) {
        std::cerr << "Error, failed to load .mp4 file." << std::endl;
        return NULL;
    }
    Mpeg4Container *pNewBox = new Mpeg4Container();
    pNewBox->m_listContents = list;

    std::vector<Box *>::iterator it = list.begin();
    while (it != list.end()) {
        Box *pBox = *it++;
        if (memcmp(pBox->m_name, "moov", 4) == 0)
            pNewBox->m_pMoovBox = pBox;
        if (memcmp(pBox->m_name, "free", 4) == 0)
            pNewBox->m_pFreeBox = pBox;
        if ((memcmp(pBox->m_name, "mdat", 4) == 0) && (!pNewBox->m_pFirstMDatBox))
            pNewBox->m_pFirstMDatBox = (Mpeg4Container *) pBox;
        if (memcmp(pBox->m_name, "ftyp", 4) == 0)
            pNewBox->m_pFTYPBox = pBox;
    }
    if (!pNewBox->m_pMoovBox) {
        std::cerr << "Error, file does not contain moov box." << std::endl;
        delete pNewBox;
        return NULL;
    }
    if (!pNewBox->m_pFirstMDatBox) {
        std::cerr << "Error, file does not contain mdat box." << std::endl;
        delete pNewBox;
        return NULL;
    }
    pNewBox->m_iFirstMDatPos = pNewBox->m_pFirstMDatBox->m_iPosition; //m_iFirstMDatPos;
    pNewBox->m_iFirstMDatPos += pNewBox->m_pFirstMDatBox->m_iHeaderSize;
    pNewBox->m_iContentSize = 0;
    it = pNewBox->m_listContents.begin();
    while (it != pNewBox->m_listContents.end()) {
        Box *pBox = *it++;
        pNewBox->m_iContentSize += pBox->size();
    }
    return pNewBox;
}

void Mpeg4Container::merge(Box *pElement)
{
    (void) pElement; // unused
    // Mpeg4 containers do not support merging."""
    std::cerr << "Cannot merge mpeg4 files" << std::endl;
    exit(0);
}

void Mpeg4Container::print_structure(const char *pIndent)
{
    // Print mpeg4 file structure recursively."""
    std::cout << "mpeg4 [" << m_iContentSize << "]" << std::endl;
    uint32_t iCount = m_listContents.size();
    std::string strIndent = pIndent;
    std::vector<Box *>::iterator it = m_listContents.begin();
    while (it != m_listContents.end()) {
        Box *pBox = *it++;
        strIndent = " ├──";
        if (--iCount <= 0)
            strIndent = " └──";
        pBox->print_structure(strIndent.c_str());
    }
}

void Mpeg4Container::save(std::fstream &fsIn, std::fstream &fsOut, int32_t)
{
    // Save mpeg4 filecontent to file.
    resize();
    uint32_t iNewPos = 0;
    std::vector<Box *>::iterator it = m_listContents.begin();
    while (it != m_listContents.end()) {
        Box *pBox = *it++;
        if (memcmp(pBox->m_name, constants::TAG_MDAT, 4) == 0) {
            iNewPos += pBox->m_iHeaderSize;
            break;
        }
        iNewPos += pBox->size();
    }
    uint32_t iDelta = iNewPos - m_iFirstMDatPos;
    it = m_listContents.begin();
    while (it != m_listContents.end()) {
        Box *pBox = *it++;
        pBox->save(fsIn, fsOut, iDelta);
    }
}
