// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_renderStates.h"
#include "nau/3d/dag_texMgr.h"
//#include <generic/dag_patchTab.h>
//#include <generic/dag_smallTab.h>
#include <stdint.h>
#include "nau/shaders/shInternalTypes.h"
#include "nau/shaders/shader_layout.h"

//#include <util/dag_globDef.h>
#include "nau/shaders/dag_renderStateId.h"
//#include <math/integer/dag_IPoint4.h>
//#include <math/dag_Tnau::math::Matrix4.h>
//#include <dag/dag_vector.h>
//#include <memory/dag_framemem.h>
#include "nau/threading/spin_lock.h"
#include <EASTL/span.h>
#include <EASTL/bonus/lru_cache.h>

struct Color4;
struct ShaderChannelId;
class BaseTexture;
class IGenLoad;

namespace shaderbindump
{
extern uint32_t get_generation();

using VarList = bindump::Mapper<shader_layout::VarList>;
using Interval = bindump::Mapper<shader_layout::Interval>;
using VariantTable = bindump::Mapper<shader_layout::VariantTable>;
using ShaderCode = bindump::Mapper<shader_layout::ShaderCode>;
using ShaderClass = bindump::Mapper<shader_layout::ShaderClass>;
using ShaderBlock = bindump::Mapper<shader_layout::ShaderBlock>;

const ShaderClass &null_shader_class(bool with_code = true);
const ShaderCode &null_shader_code();
} // namespace shaderbindump

using ScriptedShadersBinDump = bindump::Mapper<shader_layout::ScriptedShadersBinDump>;
using ScriptedShadersBinDumpV2 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV2>;
using ScriptedShadersBinDumpV3 = bindump::Mapper<shader_layout::ScriptedShadersBinDumpV3>;
using StrHolder = bindump::Mapper<bindump::StrHolder>;

enum class ShaderCodeType
{
  VERTEX,
  PIXEL,
  COMPUTE = PIXEL,
};

using ShaderBytecode = eastl::vector<uint32_t>;

struct ScriptedShadersBinDumpOwner
{
  bool load(IGenLoad &crd, int size, bool full_file_load = false);
  bool loadData(const uint8_t *dump, int size);
  // normally called from load, but can be called explicitly to restore bindump
  void initAfterLoad();

  void clear();

  size_t getDumpSize() const { return mSelfData.size(); }

  eastl::span<const uint32_t> getCode(uint32_t id, ShaderCodeType type, ShaderBytecode &tmpbuf);

  ScriptedShadersBinDump *operator->() { return mShaderDump; }
  ScriptedShadersBinDump *getDump() { return mShaderDump; }
  ScriptedShadersBinDumpV2 *getDumpV2() { return mShaderDumpV2; }
  ScriptedShadersBinDumpV3 *getDumpV3() { return mShaderDumpV3; }

  eastl::vector<int16_t> globVarIntervalIdx;
  eastl::vector<uint8_t> globIntervalNormValues;

  //auto getDecompressionDict() { return mDictionary.get(); }

private:
  struct DecompressedGroup
  {
    eastl::vector<uint8_t> decompressed_data;
    bindump::Mapper<shader_layout::ShGroup> *sh_group = nullptr;
  };
  using decompressed_groups_cache_t = eastl::lru_cache<uint16_t, DecompressedGroup>;

  ScriptedShadersBinDump *mShaderDump = nullptr;
  ScriptedShadersBinDumpV2 *mShaderDumpV2 = nullptr;
  ScriptedShadersBinDumpV3 *mShaderDumpV3 = nullptr;
  eastl::vector<uint8_t> mSelfData;

  eastl::unique_ptr<decompressed_groups_cache_t> mDecompressedGropusLru;
  nau::threading::SpinLock mDecompressedGroupsLruMutex;

  void copyDecompressedShader(const DecompressedGroup &decompressed_group, uint16_t index_in_group, ShaderBytecode &tmpbuf);
  void loadDecompressedShader(uint16_t group_id, uint16_t index_in_group, ShaderBytecode &tmpbuf);
  void storeDecompressedGroup(uint16_t group_id, DecompressedGroup &&decompressed_group);

  //struct ZstdDictionaryDeleter
  //{
  //  void operator()(struct ZSTD_DDict_s *dict) const { zstd_destroy_ddict(dict); }
  //};
  //eastl::unique_ptr<struct ZSTD_DDict_s, ZstdDictionaryDeleter> mDictionary;
};

namespace shaderbindump
{
static constexpr int MAX_BLOCK_LAYERS = 3;

extern unsigned blockStateWord;
extern shaderbindump::ShaderBlock *nullBlock[MAX_BLOCK_LAYERS];

extern bool autoBlockStateWordChange;

#if DAGOR_DBGLEVEL > 0
extern const ShaderClass *shClassUnderDebug;

void dumpShaderInfo(const ShaderClass &cls, bool dump_variants = true);
void dumpVar(const shaderbindump::VarList &vars, int var);
void dumpVars(const shaderbindump::VarList &vars);
void dumpUnusedVariants(const shaderbindump::ShaderClass &cls);

void add_exec_stcode_time(const shaderbindump::ShaderClass &cls, const __int64 &time);

bool markInvalidVariant(int shader_nid, unsigned short stat_varcode, unsigned short dyn_varcode);
bool hasShaderInvalidVariants(int shader_nid);
void resetInvalidVariantMarks();

const char *decodeVariantStr(dag::ConstSpan<shaderbindump::VariantTable::IntervalBind> p, unsigned c, String &tmp);
dag::ConstSpan<unsigned> getVariantCodesForIdx(const shaderbindump::VariantTable &vt, int code_idx);
const char *decodeStaticVariants(const shaderbindump::ShaderClass &shClass, int code_idx);
#endif

struct ShaderInterval
{
  bindump::string name;
  int value = -1;
  int valueCount = 0;
};

struct ShaderVariant
{
  bindump::vector<ShaderInterval> intervals;
  uint32_t size = 0;
  int vprId = -1;
  int fshId = -1;
};

struct ShaderStaticVariant
{
  bindump::vector<ShaderInterval> staticIntervals;
  bindump::vector<ShaderVariant> dynamicVariants;
};

struct ShaderStatistics
{
  bindump::string shaderName;
  bindump::vector<ShaderStaticVariant> staticVariants;
};

uint32_t get_dynvariant_collection_id(const shaderbindump::ShaderCode &code);
void build_dynvariant_collection_cache(eastl::vector<int> &cache);
void build_dynvariant_collection_cache(eastl::vector<int> &cache);
} // namespace shaderbindump

ScriptedShadersBinDump &shBinDumpRW(bool main = true);
const ScriptedShadersBinDump &shBinDump(bool main = true);
ScriptedShadersBinDumpOwner &shBinDumpOwner(bool main = true);
