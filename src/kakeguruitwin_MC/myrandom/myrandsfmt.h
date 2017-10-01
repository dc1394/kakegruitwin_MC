/*! \file myrandsfmt.h
    \brief SFMTを使った自作乱数クラスの宣言

    Copyright © 2017 @dc1394 All Rights Reserved.
    This software is released under the BSD 2-Clause License.
*/

#ifndef _MYRANDSFMT_H_
#define _MYRANDSFMT_H_

#pragma once

#include "../../SFMT-src-1.5.1/SFMT.h"
#include <cstdint>						// for std::int32_t, std::uint32_t, std::uint_least32_t
#include <functional>					// for std::ref
#include <random>                       // for std::random_device
#include <vector>						// for std::vector
#include <boost/range/algorithm.hpp>    // for boost::generate

namespace myrandom {
    //! A class.
    /*!
        自作乱数クラス
    */
    class MyRandSfmt final {
        // #region コンストラクタ・デストラクタ

    public:
		//! A constructor.
		/*!
			唯一のコンストラクタ
			\param min 乱数分布の最小値
			\param max 乱数分布の最大値
		*/
        MyRandSfmt(std::int32_t min, std::int32_t max);

        //! A destructor.
        /*!
            デフォルトデストラクタ
        */
        ~MyRandSfmt() = default;

        // #endregion コンストラクタ・デストラクタ

        // #region メンバ関数

        //!  A public member function.
        /*!
            [min, max]の閉区間で一様乱数を生成する
        */
        std::int32_t myrand()
        {
			return static_cast<std::int32_t>(sfmt_genrand_uint32(&sfmt) % (max_ - min_ + 1)) + min_;
        }

        // #endregion メンバ関数

        // #region メンバ変数

    private:
        //! A private static member variable (constant expression).
        /*!
            初期乱数生成用のstd::vectorのサイズ
        */
        static std::vector<std::uint_least32_t>::size_type const SIZE = 1;

		//! A private member variable.
		/*!
			乱数分布の最大値
		*/
		std::int32_t max_;
		
		//! A private member variable.
		/*!
			乱数分布の最小値
		*/
		std::int32_t min_;
		
        //! A private member variable.
        /*!
            乱数エンジン
        */
		sfmt_t sfmt;

        // #region 禁止されたコンストラクタ・メンバ関数

		//! A private constructor (deleted).
		/*!
			デフォルトコンストラクタ（禁止）
		*/
		MyRandSfmt() = delete;

        //! A private copy constructor (deleted).
        /*!
            コピーコンストラクタ（禁止）
        */
        MyRandSfmt(const MyRandSfmt &) = delete;

        //! A private member function (deleted).
        /*!
            operator=()の宣言（禁止）
            \param コピー元のオブジェクト（未使用）
            \return コピー元のオブジェクト
        */
        MyRandSfmt & operator=(const MyRandSfmt &) = delete;

        // #endregion 禁止されたコンストラクタ・メンバ関数
    };

    MyRandSfmt::MyRandSfmt(std::int32_t min, std::int32_t max)
		: max_(max),
		  min_(min)
    {
        // ランダムデバイス
        std::random_device rnd;

        // 初期化用ベクタ
        std::vector<std::uint_least32_t> v(SIZE);

        // ベクタの初期化
        // 非決定的な乱数でシード列を構築する
        boost::generate(v, std::ref(rnd));

        // 乱数エンジン
		sfmt_init_gen_rand(&sfmt, v[0]);
    }
}

#endif  // _MYRANDSFMT_H_
