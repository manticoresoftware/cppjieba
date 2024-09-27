#ifndef CPPJIEBA_MIXSEGMENT_H
#define CPPJIEBA_MIXSEGMENT_H

#include <cassert>
#include "MPSegment.hpp"
#include "HMMSegment.hpp"
#include "limonp/StringUtil.hpp"
#include "PosTagger.hpp"

namespace cppjieba {
class MixSegment: public SegmentTagged {
 public:
  MixSegment(const string& mpSegDict, const string& hmmSegDict, 
        const string& userDict = "") 
    : mpSeg_(mpSegDict, userDict), 
      hmmSeg_(hmmSegDict) {
  }
  MixSegment(const DictTrie* dictTrie, const HMMModel* model) 
    : mpSeg_(dictTrie), hmmSeg_(model) {
  }
  ~MixSegment() {
  }

  void Cut(const string& sentence, vector<string>& words) const {
    Cut(sentence, words, true);
  }
  void Cut(const string& sentence, vector<string>& words, bool hmm, CutContext * pCtx = nullptr ) const {
    vector<Word> tmp;
    Cut(sentence, tmp, hmm, pCtx);
    GetStringsFromWords(tmp, words);
  }

  template <typename STRING>
  void Cut(const STRING & sentence, vector<Word>& words, bool hmm = true, CutContext * pCtx = nullptr ) const {
    PreFilter pre_filter(symbols_, sentence);
    PreFilter::Range range;
    vector<WordRange> wrsLocal;
    vector<WordRange> & wrs = pCtx ? pCtx->wrs : wrsLocal;
	wrs.resize(0);
    wrs.reserve(sentence.size() / 2);
    while (pre_filter.HasNext()) {
      range = pre_filter.Next();
      Cut(range.begin, range.end, wrs, hmm, pCtx);
    }
    words.resize(0);
    words.reserve(wrs.size());
    GetWordsFromWordRanges(sentence, wrs, words);
  }

  void Cut(RuneStrArray::const_iterator begin, RuneStrArray::const_iterator end, vector<WordRange>& res, bool hmm, CutContext * pCtx = nullptr) const {
    if (!hmm) {
      mpSeg_.Cut(begin, end, res, MAX_WORD_LENGTH, pCtx);
      return;
    }

    vector<WordRange> wordsLocal;
    vector<WordRange> & words = pCtx ? pCtx->mixWords : wordsLocal;
    words.resize(0);

    assert(end >= begin);
    words.reserve(end - begin);
    mpSeg_.Cut(begin, end, words, MAX_WORD_LENGTH, pCtx);

    vector<WordRange> hmmResLocal;
    vector<WordRange> & hmmRes = pCtx ? pCtx->hmmRes : hmmResLocal;
	hmmRes.resize(0);
    hmmRes.reserve(end - begin);
    for (size_t i = 0; i < words.size(); i++) {
      //if mp Get a word, it's ok, put it into result
      if (words[i].left != words[i].right || (words[i].left == words[i].right && mpSeg_.IsUserDictSingleChineseWord(words[i].left->rune))) {
        res.push_back(words[i]);
        continue;
      }

      // if mp Get a single one and it is not in userdict, collect it in sequence
      size_t j = i;
      while (j < words.size() && words[j].left == words[j].right && !mpSeg_.IsUserDictSingleChineseWord(words[j].left->rune)) {
        j++;
      }

      // Cut the sequence with hmm
      assert(j - 1 >= i);
      // TODO
      hmmSeg_.Cut(words[i].left, words[j - 1].left + 1, hmmRes, pCtx);
      //put hmm result to result
      for (size_t k = 0; k < hmmRes.size(); k++) {
        res.push_back(hmmRes[k]);
      }

      //clear tmp vars
      hmmRes.resize(0);

      //let i jump over this piece
      i = j - 1;
    }
  }

  const DictTrie* GetDictTrie() const {
    return mpSeg_.GetDictTrie();
  }

  bool Tag(const string& src, vector<pair<string, string> >& res) const {
    return tagger_.Tag(src, res, *this);
  }

  string LookupTag(const string &str) const {
    return tagger_.LookupTag(str, *this);
  }

 private:
  MPSegment mpSeg_;
  HMMSegment hmmSeg_;
  PosTagger tagger_;

}; // class MixSegment

} // namespace cppjieba

#endif
