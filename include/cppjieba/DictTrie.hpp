#ifndef CPPJIEBA_DICT_TRIE_HPP
#define CPPJIEBA_DICT_TRIE_HPP

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <cmath>
#include <limits>
#include "limonp/StringUtil.hpp"
#include "limonp/Logging.hpp"
#include "Unicode.hpp"
#include "Trie.hpp"
#include "reader.h"

namespace cppjieba {

using namespace limonp;

const double MIN_DOUBLE = -3.14e+100;
const double MAX_DOUBLE = 3.14e+100;
const size_t DICT_COLUMN_NUM = 3;
const char* const UNKNOWN_TAG = "";

class DictTrie {
 public:
  enum UserWordWeightOption {
    WordWeightMin,
    WordWeightMedian,
    WordWeightMax,
  }; // enum UserWordWeightOption

  DictTrie(const string& dict_path, const string& user_dict_paths = "", UserWordWeightOption user_word_weight_opt = WordWeightMedian) {
    Init(dict_path, user_dict_paths, user_word_weight_opt);
  }

  ~DictTrie() {
    delete trie_;
  }

  bool InsertUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    if (!MakeNodeInfo(node_info, word, user_word_default_weight_, tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  bool InsertUserWord(const string& word,int freq, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    double weight = freq ? log(1.0 * freq / freq_sum_) : user_word_default_weight_ ;
    if (!MakeNodeInfo(node_info, word, weight , tag)) {
      return false;
    }
    active_node_infos_.push_back(node_info);
    trie_->InsertNode(node_info.word, &active_node_infos_.back());
    return true;
  }

  bool DeleteUserWord(const string& word, const string& tag = UNKNOWN_TAG) {
    DictUnit node_info;
    if (!MakeNodeInfo(node_info, word, user_word_default_weight_, tag)) {
      return false;
    }
    trie_->DeleteNode(node_info.word, &node_info);
    return true;
  }
  
  const DictUnit* Find(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end) const {
    return trie_->Find(begin, end);
  }

  void Find(RuneStrArray::const_iterator begin, 
        RuneStrArray::const_iterator end, 
        vector<struct Dag>&res,
        size_t max_word_len = MAX_WORD_LENGTH) const {
    trie_->Find(begin, end, res, max_word_len);
  }

  bool Find(const string& word)
  {
    const DictUnit *tmp = NULL;
    RuneStrArray runes;
    if (!DecodeRunesInString(word, runes))
    {
      XLOG(ERROR) << "Decode failed.";
    }
    tmp = Find(runes.begin(), runes.end());
    if (tmp == NULL)
    {
      return false;
    }
    else
    {
      return true;
    }
  }

  bool IsUserDictSingleChineseWord(const Rune& word) const {
    return IsIn(user_dict_single_chinese_word_, word);
  }

  double GetMinWeight() const {
    return min_weight_;
  }

  void InserUserDictNode(const string& line) {
    vector<string> buf;
    DictUnit node_info;
    Split(line, buf, " ");
    if(buf.size() == 1){
          MakeNodeInfo(node_info, 
                buf[0], 
                user_word_default_weight_,
                UNKNOWN_TAG);
        } else if (buf.size() == 2) {
          MakeNodeInfo(node_info, 
                buf[0], 
                user_word_default_weight_,
                buf[1]);
        } else if (buf.size() == 3) {
          int freq = atoi(buf[1].c_str());
          assert(freq_sum_ > 0.0);
          double weight = log(1.0 * freq / freq_sum_);
          MakeNodeInfo(node_info, buf[0], weight, buf[2]);
        }
        static_node_infos_.push_back(node_info);
        if (node_info.word.size() == 1) {
          user_dict_single_chinese_word_.insert(node_info.word[0]);
        }
  }
  
  void LoadUserDict(const vector<string>& buf) {
    for (size_t i = 0; i < buf.size(); i++) {
      InserUserDictNode(buf[i]);
    }
  }

   void LoadUserDict(const set<string>& buf) {
    std::set<string>::const_iterator iter;
    for (iter = buf.begin(); iter != buf.end(); iter++){
      InserUserDictNode(*iter);
    }
  }

  void LoadUserDict(const string& filePaths) {
    vector<string> files = limonp::Split(filePaths, "|;");
    size_t lineno = 0;
    for (size_t i = 0; i < files.size(); i++) {
      ifstream ifs(files[i].c_str());
      XCHECK(ifs.is_open()) << "open " << files[i] << " failed"; 
      string line;
      
      for (; getline(ifs, line); lineno++) {
        if (line.size() == 0) {
          continue;
        }
        InserUserDictNode(line);
      }
    }
  }


 private:
  void Init(const string& dict_path, const string& user_dict_paths, UserWordWeightOption user_word_weight_opt) {
    LoadDict(dict_path);
    freq_sum_ = CalcFreqSum(static_node_infos_);
    CalculateWeight(static_node_infos_, freq_sum_);
    SetStaticWordWeights(user_word_weight_opt);

    if (user_dict_paths.size()) {
      LoadUserDict(user_dict_paths);
    }
    Shrink(static_node_infos_);
    CreateTrie(static_node_infos_);
  }
  
  void CreateTrie(const vector<DictUnit>& dictUnits) {
    assert(dictUnits.size());
    vector<Unicode> words;
    vector<const DictUnit*> valuePointers;
    words.resize(dictUnits.size());
    valuePointers.resize(dictUnits.size());
    for (size_t i = 0 ; i < dictUnits.size(); i ++) {
      words[i] = dictUnits[i].word;
      valuePointers[i] = &dictUnits[i];
    }

    trie_ = new Trie(words, valuePointers);
  }

  


  bool MakeNodeInfo(DictUnit& node_info,
        const string& word, 
        double weight, 
        const string& tag) {
    if (!DecodeRunesInString(word, node_info.word)) {
      XLOG(ERROR) << "Decode " << word << " failed.";
      return false;
    }
    node_info.weight = weight;
    node_info.tag = tag;
    return true;
  }

    bool MakeNodeInfo(DictUnit& node_info,
        const char * word,
        double weight,
        const char * tag )
  {
    if (!DecodeRunesInString(word, node_info.word)) {
      XLOG(ERROR) << "Decode " << word << " failed.";
      return false;
    }
    node_info.weight = weight;
    node_info.tag = tag;
    return true;
  }

  void LoadDict(const string& filePath)
  {
    FileUtil::FileReader_c tReader;
    bool bOpenOk = tReader.Open(filePath);
    XCHECK(bOpenOk) << "open " << filePath << " failed.";

    std::streamsize fileSize = 0;
    std::ifstream tmpFile ( filePath, std::ios::binary | std::ios::ate );
    if ( tmpFile.is_open() )
        fileSize = tmpFile.tellg();

    const int AVG_LINE_LEN = 15;
    static_node_infos_.reserve ( fileSize/AVG_LINE_LEN );

    DictUnit node_info;
    const int MAX_LINE_LEN = 1024;
    char dBuffer[MAX_LINE_LEN];
    char * dValues[DICT_COLUMN_NUM];

    int iLen = 0;
    while ( ( iLen = tReader.GetLine ( dBuffer, sizeof(dBuffer) ) )>=0 )
    {
      bool bSplitOk = SplitText ( dBuffer, iLen, dValues, DICT_COLUMN_NUM );
      XCHECK(bSplitOk) << "split result illegal, line:" << dBuffer;
      MakeNodeInfo(node_info, 
            dValues[0], 
            atof(dValues[1]), 
            dValues[2]);
      static_node_infos_.push_back(node_info);
    }
  }

  void SetStaticWordWeights(UserWordWeightOption option) {
    XCHECK(!static_node_infos_.empty());
    std::vector<int> indices(static_node_infos_.size());
    for ( size_t i = 0; i < indices.size(); i++ )
        indices[i] = i;
  
    std::sort(indices.begin(), indices.end(), [this](size_t i1, size_t i2) { return static_node_infos_[i1].weight < static_node_infos_[i2].weight; });

    min_weight_ = static_node_infos_[indices.front()].weight;
    max_weight_ = static_node_infos_[indices.back()].weight;
    median_weight_ = static_node_infos_[indices[indices.size() / 2]].weight;
    switch (option) {
     case WordWeightMin:
       user_word_default_weight_ = min_weight_;
       break;
     case WordWeightMedian:
       user_word_default_weight_ = median_weight_;
       break;
     default:
       user_word_default_weight_ = max_weight_;
       break;
    }
  }

  double CalcFreqSum(const vector<DictUnit>& node_infos) const {
    double sum = 0.0;
    for (size_t i = 0; i < node_infos.size(); i++) {
      sum += node_infos[i].weight;
    }
    return sum;
  }

  void CalculateWeight(vector<DictUnit>& node_infos, double sum) const {
    assert(sum > 0.0);
    for (size_t i = 0; i < node_infos.size(); i++) {
      DictUnit& node_info = node_infos[i];
      assert(node_info.weight > 0.0);
      node_info.weight = log(double(node_info.weight)/sum);
    }
  }

  void Shrink(vector<DictUnit>& units) const {
    vector<DictUnit>(units.begin(), units.end()).swap(units);
  }

  vector<DictUnit> static_node_infos_;
  deque<DictUnit> active_node_infos_; // must not be vector
  Trie * trie_;

  double freq_sum_;
  double min_weight_;
  double max_weight_;
  double median_weight_;
  double user_word_default_weight_;
  unordered_set<Rune> user_dict_single_chinese_word_;
};
}

#endif
