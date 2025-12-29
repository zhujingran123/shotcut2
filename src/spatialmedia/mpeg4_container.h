#pragma once
/*****************************************************************************
 * 
 * Copyright 2016 Varol Okan. All rights reserved.
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

// 【文件说明】：MPEG4处理类头文件
// 【功能】：定义MP4/MOV文件加载和原子操作的顶层容器类接口
// 【特性】：支持完整的MP4文件结构管理和关键原子识别
#include "constants.h"
#include "container.h"
#include "box.h"

// 【类说明】：MP4容器顶层管理类（继承自Container）
// 【功能】：表示整个MP4文件的容器，管理文件级别的操作和关键原子
// 【特性】：支持文件加载、结构验证、偏移量计算和完整保存
class Mpeg4Container : public Container
{
public:
    // 【构造函数】：初始化MP4容器
    Mpeg4Container ( );
    
    // 【析构函数】：清理MP4容器资源
    virtual ~Mpeg4Container ( );

    // 【静态方法】：MP4文件加载
    // 【功能】：从文件流加载整个MP4文件结构（静态工厂方法）
    // 【参数】：fs - 输入文件流
    // 【返回值】：成功加载的MP4容器指针，失败返回NULL
    static Mpeg4Container *load ( std::fstream &fs );

    // 【容器操作方法】
    
    // 【功能】：合并操作（MP4容器不支持合并）
    // 【说明】：MP4文件格式不允许简单合并，需要复杂的重映射操作
    void merge ( Box * );
    
    // 【功能】：打印MP4文件结构（树形显示）
    // 【参数】：p - 缩进前缀字符串，用于层次显示
    virtual void print_structure ( const char *p="" );
    
    // 【功能】：保存MP4容器到输出文件流
    // 【说明】：处理媒体数据偏移量更新并递归保存所有原子
    // 【参数】：fsIn - 输入文件流（读取原始数据）
    //          fsOut - 输出文件流（写入新文件）
    //          iDelta - 偏移量变化（由内部计算）
    virtual void save ( std::fstream &fsIn, std::fstream &fsOut, int32_t iDelta );

public:
  // 【公有成员】：关键原子指针
  
  // 电影元数据原子指针（moov）
  // - 必需原子：包含视频轨道的所有元数据信息
  // - 包含：轨道信息、时长、样本表、编解码参数等
  Box *m_pMoovBox;
  
  // 空闲空间原子指针（free）
  // - 可选原子：标记文件中未使用的空间
  // - 可用于后续的文件修改和扩展
  Box *m_pFreeBox;
  
  // 文件类型原子指针（ftyp）
  // - 通常存在：标识MP4文件的兼容性和版本
  // - 包含：品牌标识、兼容版本号等
  Box *m_pFTYPBox;
  
  // 第一个媒体数据原子指针（mdat）
  // - 必需原子：存储实际的视频和音频数据
  // - 注意：MP4文件可能有多个mdat原子，这里记录第一个
  Mpeg4Container *m_pFirstMDatBox;
  
  // 第一个媒体数据的实际内容起始位置
  // - 计算方式：m_pFirstMDatBox->m_iPosition + m_pFirstMDatBox->m_iHeaderSize
  // - 用途：在文件保存时计算偏移量变化，用于更新索引表
  uint32_t m_iFirstMDatPos;
};
