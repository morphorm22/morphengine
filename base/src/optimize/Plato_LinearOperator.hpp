/*
 * Plato_LinearOperator.hpp
 *
 *  Created on: Oct 21, 2017
 */

/*
//@HEADER
// *************************************************************************
//   Plato Engine v.1.0: Copyright 2018, National Technology & Engineering
//                    Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Sandia Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact the Plato team (plato3D-help@sandia.gov)
//
// *************************************************************************
//@HEADER
*/

#ifndef PLATO_LINEAROPERATOR_HPP_
#define PLATO_LINEAROPERATOR_HPP_

namespace Plato
{

struct Hessian
{
    enum type_t
    {
        ANALYTICAL = 1, LBFGS = 2, DISABLED = 3
    };
};
// struct Hessian

} // namespace Plato

namespace Plato
{

template<typename ScalarType, typename OrdinalType>
class StateData;
template<typename ScalarType, typename OrdinalType>
class MultiVector;

/******************************************************************************//**
 * @brief Linear operator interface, e.g. analytical Hessian interface
**********************************************************************************/
template<typename ScalarType, typename OrdinalType = size_t>
class LinearOperator
{
public:
    /******************************************************************************//**
     * @brief Default destructor
    **********************************************************************************/
    virtual ~LinearOperator()
    {
    }

    /******************************************************************************//**
     * @brief Update state data since new control was accepted.
     * @param [in] aStateData data structure with core state data
    **********************************************************************************/
    virtual void update(const Plato::StateData<ScalarType, OrdinalType> & aStateData) = 0;

    /******************************************************************************//**
     * @brief Apply vector to Hessian operator.
     * @param [in] aControl optimization variables
     * @param [in] aVector descent direction
     * @param [in,out] aOutput application of vector to Hessian operator
    **********************************************************************************/
    virtual void apply(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                       const Plato::MultiVector<ScalarType, OrdinalType> & aVector,
                       Plato::MultiVector<ScalarType, OrdinalType> & aOutput) = 0;
};
// class LinearOperator

}
// namespace Plato

#endif /* PLATO_LINEAROPERATOR_HPP_ */
