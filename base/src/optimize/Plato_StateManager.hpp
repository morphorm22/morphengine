/*
 * Plato_StateManager.hpp
 *
 *  Created on: Oct 22, 2017
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
// *************************************************************************
//@HEADER
*/

#ifndef PLATO_STATEMANAGER_HPP_
#define PLATO_STATEMANAGER_HPP_

namespace Plato
{

template<typename ScalarType, typename OrdinalType>
class MultiVector;

template<typename ScalarType, typename OrdinalType = size_t>
class StateManager
{
public:
    virtual ~StateManager()
    {
    }

    virtual ScalarType evaluateObjective(const Plato::MultiVector<ScalarType, OrdinalType> & aControl) = 0;
    virtual void computeGradient(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                                 Plato::MultiVector<ScalarType, OrdinalType> & aOutput) = 0;
    virtual void applyVectorToHessian(const Plato::MultiVector<ScalarType, OrdinalType> & aControl,
                                      const Plato::MultiVector<ScalarType, OrdinalType> & aVector,
                                      Plato::MultiVector<ScalarType, OrdinalType> & aOutput) = 0;

    virtual ScalarType getCurrentObjectiveValue() const = 0;
    virtual void setCurrentObjectiveValue(const ScalarType & aInput) = 0;
    virtual const Plato::MultiVector<ScalarType, OrdinalType> & getTrialStep() const = 0;
    virtual void setTrialStep(const Plato::MultiVector<ScalarType, OrdinalType> & aInput) = 0;
    virtual const Plato::MultiVector<ScalarType, OrdinalType> & getCurrentControl() const = 0;
    virtual void setCurrentControl(const Plato::MultiVector<ScalarType, OrdinalType> & aInput) = 0;
    virtual const Plato::MultiVector<ScalarType, OrdinalType> & getCurrentGradient() const = 0;
    virtual void setCurrentGradient(const Plato::MultiVector<ScalarType, OrdinalType> & aInput) = 0;
    virtual const Plato::MultiVector<ScalarType, OrdinalType> & getControlLowerBounds() const = 0;
    virtual void setControlLowerBounds(const Plato::MultiVector<ScalarType, OrdinalType> & aInput) = 0;
    virtual const Plato::MultiVector<ScalarType, OrdinalType> & getControlUpperBounds() const = 0;
    virtual void setControlUpperBounds(const Plato::MultiVector<ScalarType, OrdinalType> & aInput) = 0;
};

} // namespace Plato

#endif /* PLATO_STATEMANAGER_HPP_ */
