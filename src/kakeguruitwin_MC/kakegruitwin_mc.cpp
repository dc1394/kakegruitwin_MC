/*! \file kakegruitwin_mc.cpp
    \brief 賭ケグルイ双(1)のpp.100-124の内容を、モンテカルロ・シミュレーションで確かめる

    Copyright © 2017 @dc1394 All Rights Reserved.
    This software is released under the BSD 2-Clause License.
*/

#include "../checkpoint/checkpoint.h"
#include "myrandom/myrand.h"
#include <array>                       	// for std::array
#include <cstdint>  	               	// for std::uint32_t
#include <functional>                   // for std::hash
#include <iomanip>		               	// for std::setiosflags, std::setprecision
#include <iostream> 	               	// for std::cout
#include <string>                      	// for std::string
#include <utility>                      // for std::move
#ifdef _CHECK_PARALLEL_PERFORM
    #include <vector>   	            // for std::vector
#endif
#include <boost/container/flat_map.hpp>	// for boost::container::flat_map
#include <tbb/concurrent_hash_map.h>    // for tbb::concurrent_hash_map
#include <tbb/concurrent_vector.h>     	// for tbb::concurrent_vector
#ifdef __INTEL_COMPILER
    #include <cilk/cilk.h>              // for cilk_for
#else
    #include <tbb/parallel_for.h>       // for tbb::parallel_for
#endif

namespace {
    //! A global variable (constant expression).
    /*!
        モンテカルロシミュレーションの試行回数
    */
    static auto constexpr MCMAX = 1000000U;

    //! A global variable (constant expression).
    /*!
        UかDの文字列の長さ
    */
    static auto constexpr RANDNUMTABLELEN = 100U;

    //! A global variable (constant).
    /*!
        UとDの文字列の可能な集合の配列
    */
    static std::array<std::string, 8U> const udarray = { "DDD", "DDU", "DUD", "DUU", "UDD", "UDU", "UUD", "UUU" };

    //! A typedef.
    /*!
        文字列とその文字列に対応する出現回数のstd::map
    */
    using mymap = boost::container::flat_map<std::string, std::uint32_t>;

    //! A typedef.
    /*!
        文字列のペア
    */
    using strpair = std::pair<std::string, std::string>;
    
    //! A struct.
    /*!
        strpairを扱うハッシュと比較操作を定義する構造体
    */
    struct MyHashCompare final {
        //! A public static member function.
        /*!
            strpairからハッシュを生成する
            \param sp 文字列のペア
            \return 与えられたstrpairのハッシュ
        */
        static std::size_t hash(strpair const & sp)
        {
            auto const h1 = std::hash<std::string>()(sp.first);
            auto const h2 = std::hash<std::string>()(sp.second);
            return h1 ^ (h2 << 1);
        }

        //! A public static member function.
        /*!
            二つのstrpairを比較する
            \param lhs 左辺のstrpair
            \param rhs 右辺のstrpair
            \return 文字列のペアが等しい場合にはtrue
        */
        static bool equal(strpair const & lhs, strpair const & rhs)
        {
            return lhs == rhs;
        }
    };

    //! A typedef.
    /*!
        各文字列の順列に対応する勝利回数の結果を格納するtbb::concurrent_hash_map
    */
    using myhashmap = tbb::concurrent_hash_map<strpair, std::uint32_t, MyHashCompare>;
    
    //! A typedef.
    /*!
        文字列のペアと、どちらの文字列が勝ったかのstd::pair
    */
    using mymap2 = boost::container::flat_map<strpair, bool>;

    //! A typedef.
    /*!
        文字列のペアと、文字列の勝利数のstd::pair
    */
    using mymap3 = boost::container::flat_map<strpair, std::uint32_t>;
    
    //! A function.
    /*!
        文字列の可能な順列を列挙する
        \return 文字列の可能な順列を列挙したstd::array
    */
    std::array<strpair, 56U> makecombination();

    //! A global variable (constant).
    /*!
        udarrayから二つを抽出したときの可能な順列の配列
    */
    static std::array<strpair, 56U> const cbarray = makecombination();

#ifdef _CHECK_PARALLEL_PERFORM
    //! A function.
    /*!
        文字列のペアの、前者が勝利した回数を集計する
        \param mcresultwinningavg 文字列のペアと、どちらの文字列が勝ったかの結果が格納された連想配列の可変長配列
        \return 文字列のペアの、前者が勝利した回数が格納された連想配列
    */
    mymap3 aggregateWinningAvg(std::vector<mymap2> const & mcresultwinningavg);
#endif

    //! A function.
    /*!
        文字列のペアの、前者が勝利した回数を集計する
        \param mcresultwinningavg 文字列のペアと、どちらの文字列が勝ったかの結果が格納された連想配列の可変長配列
        \return 文字列のペアの、前者が勝利した回数が格納された連想配列
    */
    mymap3 aggregateWinningAvg(tbb::concurrent_vector<mymap2> const & mcresultwinningavg);

    //! A function.
    /*!
        UDのランダム文字列を生成する
        \param mr 自作乱数クラスのオブジェクト
        \return UDのランダム文字列を格納したstd::string
    */
    inline auto makerandomudstr(myrandom::MyRand & mr);

    //! A function.
    /*!
        モンテカルロ・シミュレーションを行う
        \return 期待値と、どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果のstd::pair    
    */
    std::pair<std::vector<mymap>, std::vector<mymap2> > montecarlo();

    //! A function.
    /*!
        モンテカルロ・シミュレーションをTBBで並列化して行う
        \return 期待値と、どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果のstd::pair
    */
    std::pair< tbb::concurrent_vector<mymap>, tbb::concurrent_vector<mymap2> > montecarloTBB();

    //! A function.
    /*!
        期待値に対するモンテカルロ・シミュレーションの実装
        \param mr 自作乱数クラスのオブジェクト
        \return 期待値に対するモンテカルロ・シミュレーションの結果が格納された連想配列
    */
    mymap montecarloImplAvg(myrandom::MyRand & mr);

    //! A function.
    /*!
        文字列のペアのうち、どちらの文字列が先に出現したかのモンテカルロ・シミュレーションの実装
        \param mr 自作乱数クラスのオブジェクト
        \return 文字列のペアのうち、どちらの文字列が先に出現したかのモンテカルロ・シミュレーションの結果が格納された連想配列
    */
    mymap2 montecarloImplWinningAvg(myrandom::MyRand & mr);

    //! A function.
    /*!
        UとDのランダム文字列から与えられた文字列の位置を検索し、文字列の末尾の位置を与える
        \param str 検索する文字列
        \param udstr UとDのランダム文字列
        \return 検索された文字列の末尾の位置
    */
    inline std::uint32_t myfind(std::string const & str, std::string const & udstr);

    //! A template function.
    /*!
        期待値に対するモンテカルロ・シミュレーションの和を計算する
        \return 期待値に対するモンテカルロ・シミュレーションの結果の和の連想配列
    */
    template <typename T>
    mymap sumMontecarloAvg(T const & mcresultavg);
}

int main()
{
    checkpoint::CheckPoint cp;

    cp.checkpoint("処理開始", __LINE__);

#ifdef _CHECK_PARALLEL_PERFORM
    {
        // モンテカルロ・シミュレーションの結果を代入
        auto const mcresult(montecarlo());

        // 各文字列のペアに対する勝率を計算する
        auto const trialwinningavg(aggregateWinningAvg(mcresult.second));
    }

    cp.checkpoint("並列化無効", __LINE__);
#endif

    // モンテカルロ・シミュレーションの結果を代入
    auto const mcresultTBB(montecarloTBB());
    
    // 各文字列のペアに対する勝率を計算する
    auto const trialwinningavg(aggregateWinningAvg(mcresultTBB.second));

    cp.checkpoint("並列化有効", __LINE__);

    // 期待値に対するモンテカルロ・シミュレーションの結果の和を計算する
    auto const trialavg(sumMontecarloAvg(mcresultTBB.first));
    
    // 各文字列に対する期待値の表示
    std::cout << std::setprecision(1) << std::setiosflags(std::ios::fixed);
    for (auto && itr = trialavg.begin(); itr != trialavg.end(); ++itr) {
        std::cout << itr->first
                  << " が出るまでの期待値: "
                  << static_cast<double>(itr->second) / static_cast<double>(MCMAX)
                  << "回\n";
    }
    
    // 各文字列のペアに対する勝率の表示
    std::cout << "\n    ";
    for (auto i = 0; i < 8; i++) {
        std::cout << udarray[i] << "  ";
    }
    std::cout << '\n';

    auto && itr = trialwinningavg.begin();
    auto const len = udarray.size();
    for (auto i = 0U; i < len; i++) {
        std::cout << itr->first.first << ' ';
        for (auto j = 0U; j < len; j++) {
            if (i == j) {
                std::cout << "     ";
            }
            else {
                std::cout << static_cast<double>(itr->second) / static_cast<double>(MCMAX) * 100.0
                          << ' ';
                ++itr;
            }
        }
        std::cout << '\n';
    }

    cp.checkpoint("それ以外の処理", __LINE__);

    cp.checkpoint_print();

    return 0;
}

namespace {
#ifdef _CHECK_PARALLEL_PERFORM
    mymap3 aggregateWinningAvg(std::vector<mymap2> const & mcresultwinningavg)
    {
        // 各文字列の順列に対応する勝利回数の結果を格納するboost::container::flat_map
        mymap3 trialwinningavg;

        // tbb::concurrent_hash_mapの初期化
        for (auto const & sp : cbarray) {
            trialwinningavg[sp] = 0U;
        }

        // 試行回数分繰り返す
        for (auto const & mcr : mcresultwinningavg) {
            for (auto && itr = mcr.begin(); itr != mcr.end(); ++itr) {
                if (itr->second) {
                    trialwinningavg[itr->first]++;
                }
            }
        }

        return trialwinningavg;
    }
#endif

    mymap3 aggregateWinningAvg(tbb::concurrent_vector<mymap2> const & mcresultwinningavg)
    {
        // 各文字列の順列に対応する勝利回数の結果を格納するtbb::concurrent_hash_map
        myhashmap trial;

        // tbb::concurrent_hash_mapの初期化
        for (auto const & sp : cbarray) {
            myhashmap::accessor a;
            trial.insert(a, sp);
            a->second = 0U;
        }

        // 試行回数分繰り返す
#ifdef __INTEL_COMPILER
        cilk_for (auto i = 0U; i < MCMAX; i++) {
#else
        tbb::parallel_for(
            tbb::blocked_range<std::uint32_t>(0U, MCMAX),
            [&mcresultwinningavg, &trial](auto const & range) {
            for (auto && i = range.begin(); i != range.end(); ++i) {
#endif
                auto const mcr = mcresultwinningavg[i];
                for (auto && itr = mcr.begin(); itr != mcr.end(); ++itr) {
                    if (itr->second) {
                        myhashmap::accessor a;
                        trial.insert(a, itr->first);
                        a->second++;
                    }
                }
            }
#ifndef __INTEL_COMPILER
        });
#endif
        // boost::container::flat_mapに計算結果を複写
        boost::container::flat_map<strpair, std::uint32_t> trialwinningavg;
        for (auto && res : trial) {
            trialwinningavg[res.first] = res.second;
        }

        return trialwinningavg;
    }


    std::array<strpair, 56U> makecombination()
    {
        // 全ての可能な順列を収納する配列
        std::array<strpair, 56U> cb;

        // カウンタ
        auto cnt = 0;

        // 全ての可能な順列を列挙
        auto const len = udarray.size();
        for (auto i = 0U; i < len; i++) {
            for (auto j = 0U; j < len; j++) {
                if (i != j) {
                    cb[cnt++] = std::make_pair(udarray[i], udarray[j]);
                }
            }
        }

        return cb;
    }

    auto makerandomudstr(myrandom::MyRand & mr)
    {
        // UDのランダム文字列を格納するstd::string
        std::string udstring(RANDNUMTABLELEN, '\0');

        // UDのランダム文字列を格納
        for (auto && c : udstring) {
            c = mr.myrand() > 3 ? 'U' : 'D';
        }

		// UDのランダム文字列を返す
        return udstring;
    }

#ifdef _CHECK_PARALLEL_PERFORM
    std::pair<std::vector<mymap>, std::vector<mymap2> > montecarlo()
    {
        // 期待値に対するモンテカルロ・シミュレーションの結果を格納するための可変長配列
        std::vector<mymap> mcresultavg;

        // どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果を格納するための可変長配列
        std::vector<mymap2> mcresultwinningavg;

        // MCMAX個の容量を確保
        mcresultavg.reserve(MCMAX);
        mcresultwinningavg.reserve(MCMAX);
        
        // 自作乱数クラスを初期化
        myrandom::MyRand mr(1, 6);

        // 試行回数分繰り返す
        for (auto i = 0U; i < MCMAX; i++) {
            // 期待値に対するモンテカルロ・シミュレーションの結果を代入
            mcresultavg.push_back(montecarloImplAvg(mr));

            // どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果を代入
            mcresultwinningavg.push_back(montecarloImplWinningAvg(mr));
        }

        return std::make_pair(std::move(mcresultavg), std::move(mcresultwinningavg));
    }
#endif    

    std::pair<tbb::concurrent_vector<mymap>, tbb::concurrent_vector<mymap2> > montecarloTBB()
    {
        // 期待値に対するモンテカルロ・シミュレーションの結果を格納するための可変長配列
        tbb::concurrent_vector<mymap> mcresultavg;

        // どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果を格納するための可変長配列
        tbb::concurrent_vector<mymap2> mcresultwinningavg;

        // MCMAX個の容量を確保
        mcresultavg.reserve(MCMAX);
        mcresultwinningavg.reserve(MCMAX);
        
        // 試行回数分繰り返す
#ifdef __INTEL_COMPILER
        cilk_for (auto i = 0U; i < MCMAX; i++) {
#else
        tbb::parallel_for(
            0U,
            MCMAX,
            1U,
            [&mcresultavg, &mcresultwinningavg](auto) {
#endif
                // 自作乱数クラスを初期化
                myrandom::MyRand mr(1, 6);

                // 期待値に対するモンテカルロ・シミュレーションの結果を代入
                mcresultavg.push_back(montecarloImplAvg(mr));

                // どちらの文字列が先に出現したかどうかのモンテカルロ・シミュレーションの結果を代入
                mcresultwinningavg.push_back(montecarloImplWinningAvg(mr));
#ifdef __INTEL_COMPILER
        }
#else
        });
#endif

        return std::make_pair(std::move(mcresultavg), std::move(mcresultwinningavg));
    }

    mymap montecarloImplAvg(myrandom::MyRand & mr)
    {
        // UDのランダム文字列
        auto const udstr(makerandomudstr(mr));

        // 検索結果のstd::map
        mymap result;

        // 文字列が最初に出現するのは何文字目かを検索し結果を代入
        for (auto const & str : udarray) {
            result.insert(std::make_pair(str, myfind(str, udstr)));
        }

        return result;
    }
        
    mymap2 montecarloImplWinningAvg(myrandom::MyRand & mr)
    {
        // UDのランダム文字列
        auto const udstr(makerandomudstr(mr));

        // 検索結果のstd::map
        mymap2 result;

        // どちらの文字列が先に出現したかの結果を代入
        for (auto const & sp : cbarray) {
            result.insert(std::make_pair(sp, myfind(sp.first, udstr) < myfind(sp.second, udstr)));
        }

        // 検索結果を返す
        return result;
    }
        
    std::uint32_t myfind(std::string const & str, std::string const & udstr)
    {
        // 文字列の位置を検索
        auto const pos = udstr.find(str);
        
        // posをその文字列の末尾の位置に変換
        // もし文字列が見つかっていなかった場合はRANDNUMTABLELENに変換
        return pos != std::string::npos ? static_cast<std::uint32_t>(pos + 3) : RANDNUMTABLELEN;
    }
    
    template <typename T>
    mymap sumMontecarloAvg(T const & mcresultavg)
    {
        // 各文字列に対して、期待値に対するモンテカルロ・シミュレーションの結果の和を格納するstd::map
        mymap trial;

        // std::mapの初期化
        for (auto const & str : udarray) {
            trial[str] = 0;
        }

        // 試行回数分繰り返す
        for (auto const & mcr : mcresultavg) {
            for (auto && itr = mcr.begin(); itr != mcr.end(); ++itr) {
                trial[itr->first] += itr->second;
            }
        }

        return trial;
    }
}
