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

/*
 * Plato_ParticleSwarmAlgorithmALPSO.hpp
 *
 *  Created on: Jan 24, 2019
*/

#pragma once

#include <string>

#include "Plato_DataFactory.hpp"
#include "Plato_CommWrapper.hpp"
#include "Plato_ParticleSwarmDataMng.hpp"
#include "Plato_ParticleSwarmOperations.hpp"
#include "Plato_ParticleSwarmIO_Utilities.hpp"
#include "Plato_ParticleSwarmStageMngALPSO.hpp"

namespace Plato
{

/******************************************************************************//**
 * @brief Interface for Augmented Lagrangian Particle Swarm Optimization (ALPSO) algorithm
**********************************************************************************/
template<typename ScalarType, typename OrdinalType = size_t>
class ParticleSwarmAlgorithmALPSO
{
public:
    /******************************************************************************//**
     * @brief Constructor
     * @param [in] aFactory PSO algorithm data factory
     * @param [in] aObjective gradient free objective function interface
     * @param [in] aConstraints list of gradient free constraint interface
    **********************************************************************************/
    ParticleSwarmAlgorithmALPSO(const std::shared_ptr<Plato::DataFactory<ScalarType, OrdinalType>> & aFactory,
                                const std::shared_ptr<Plato::GradFreeCriterion<ScalarType, OrdinalType>> & aObjective,
                                const std::shared_ptr<Plato::GradFreeCriteriaList<ScalarType, OrdinalType>> & aConstraints) :
            mParticleDiagnostics(false),
            mAlgorithmDiagnostics(false),
            mStdDevStoppingTolActive(true),
            mNumIterations(0),
            mMaxNumAugLagOuterIterations(1e3),
            mMeanBestAugLagFuncTolerance(5e-4),
            mStdDevBestAugLagFuncTolerance(1e-6),
            mGlobalBestAugLagFuncTolerance(1e-10),
            mTrustRegionMultiplierTolerance(1e-8),
            mStopCriterion(Plato::particle_swarm::DID_NOT_CONVERGE),
            mOutputData(aConstraints->size()),
            mCustomOutput(std::make_shared<Plato::CustomOutput<ScalarType, OrdinalType>>()),
            mStageMng(std::make_shared<Plato::ParticleSwarmStageMngALPSO<ScalarType, OrdinalType>>(aFactory, aObjective, aConstraints)),
            mOptimizer(std::make_shared<Plato::ParticleSwarmAlgorithmBCPSO<ScalarType, OrdinalType>>(aFactory, mStageMng))
    {
        mOptimizer->setMaxNumIterations(10); /* augmented Lagrangian subproblem iterations */
    }

    /******************************************************************************//**
     * @brief Destructor
    **********************************************************************************/
    ~ParticleSwarmAlgorithmALPSO()
    {
    }

    /******************************************************************************//**
     * @brief Enable output of ALPSO algorithm diagnostics (i.e. optimization problem status)
    **********************************************************************************/
    void enableDiagnostics()
    {
        mAlgorithmDiagnostics = true;
        mOptimizer->enableDiagnostics();
    }

    /******************************************************************************//**
     * @brief Enable output of particle data diagnostics (i.e. solution status)
    **********************************************************************************/
    void enableParticleDiagnostics()
    {
        mParticleDiagnostics = true;
    }

    /******************************************************************************//**
     * @brief Disables stopping tolerance based on the standard deviation of the augmented Lagrangian function
    **********************************************************************************/
    void disableStdDevStoppingTolerance()
    {
        mStdDevStoppingTolActive = false;
        mOptimizer->disableStdDevStoppingTolerance();
    }

    /******************************************************************************//**
     * @brief Set maximum number of outer iterations
     * @param [in] aInput maximum number of outer iterations
    **********************************************************************************/
    void setMaxNumOuterIterations(const OrdinalType & aInput)
    {
        mMaxNumAugLagOuterIterations = aInput;
    }

    /******************************************************************************//**
     * @brief Set maximum number of inner iterations
     * @param [in] aInput maximum number of inner iterations
    **********************************************************************************/
    void setMaxNumInnerIterations(const OrdinalType & aInput)
    {
        mOptimizer->setMaxNumIterations(aInput);
    }

    /******************************************************************************//**
     * @brief Set maximum number of consecutive failures
     * @param [in] aInput maximum number of consecutive failures
    **********************************************************************************/
    void setMaxNumConsecutiveFailures(const OrdinalType & aInput)
    {
        mOptimizer->setMaxNumConsecutiveFailures(aInput);
    }

    /******************************************************************************//**
     * @brief Set maximum number of consecutive successes
     * @param [in] aInput maximum number of consecutive successes
    **********************************************************************************/
    void setMaxNumConsecutiveSuccesses(const OrdinalType & aInput)
    {
        mOptimizer->setMaxNumConsecutiveSuccesses(aInput);
    }

    /******************************************************************************//**
     * @brief Set time step used to compute particle velocities
     * @param [in] aInput time step used to compute particle velocities
    **********************************************************************************/
    void setTimeStep(const ScalarType & aInput)
    {
        mOptimizer->setTimeStep(aInput);
    }

    /******************************************************************************//**
     * @brief Set inertia multiplier
     * @param [in] aInput inertia multiplier
    **********************************************************************************/
    void setInertiaMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setInertiaMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set random number multiplier used to update the particle positions.
     * The random number multiplier is used to find an unique particle.
     * @param [in] aInput random number multiplier
    **********************************************************************************/
    void setRandomNumMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setRandomNumMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set cognitive behavior multiplier
     * @param [in] aInput cognitive behavior multiplier
    **********************************************************************************/
    void setCognitiveBehaviorMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setCognitiveBehaviorMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set social behavior multiplier
     * @param [in] aInput social behavior multiplier
    **********************************************************************************/
    void setSocialBehaviorMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setSocialBehaviorMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set penalty expansion multiplier
     * @param [in] aInput penalty expansion multiplier
    **********************************************************************************/
    void setPenaltyExpansionMultiplier(const ScalarType & aInput)
    {
        mStageMng->setPenaltyExpansionMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set penalty contraction multiplier
     * @param [in] aInput penalty contraction multiplier
    **********************************************************************************/
    void setPenaltyContractionMultiplier(const ScalarType & aInput)
    {
        mStageMng->setPenaltyContractionMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set upper bound on penalty multipliers
     * @param [in] aInput upper bound on penalty multipliers
    **********************************************************************************/
    void setPenaltyMultiplierUpperBound(const ScalarType & aInput)
    {
        mStageMng->setPenaltyMultiplierUpperBound(aInput);
    }

    /******************************************************************************//**
     * @brief Set feasibility inexactness tolerance
     * @param [in] aInput feasibility inexactness tolerance
    **********************************************************************************/
    void setFeasibilityInexactnessTolerance(const ScalarType & aInput)
    {
        mStageMng->setFeasibilityInexactnessTolerance(aInput);
    }

    /******************************************************************************//**
     * @brief Set stopping tolerance on the mean of the best augmented Lagrangian function value
     * @param [in] aInput stopping tolerance
    **********************************************************************************/
    void setMeanAugLagFuncTolerance(const ScalarType & aInput)
    {
        mMeanBestAugLagFuncTolerance = aInput;
    }

    /******************************************************************************//**
     * @brief Set stopping tolerance on the standard deviation of the best augmented Lagrangian function value
     * @param [in] aInput stopping tolerance
    **********************************************************************************/
    void setStdDevAugLagFuncTolerance(const ScalarType & aInput)
    {
        mStdDevBestAugLagFuncTolerance = aInput;
    }

    /******************************************************************************//**
     * @brief Set stopping tolerance on the global best augmented Lagrangian function value
     * @param [in] aInput stopping tolerance
    **********************************************************************************/
    void setBestAugLagFuncTolerance(const ScalarType & aInput)
    {
        mGlobalBestAugLagFuncTolerance = aInput;
    }

    /******************************************************************************//**
     * @brief Set stopping tolerance based on the trust region multiplier
     * @param [in] aInput stopping tolerance
    **********************************************************************************/
    void setTrustRegionMultiplierTolerance(const ScalarType & aInput)
    {
        mTrustRegionMultiplierTolerance = aInput;
    }

    /******************************************************************************//**
     * @brief Set trust region expansion multiplier
     * @param [in] aInput trust region expansion multiplier
    **********************************************************************************/
    void setTrustRegionExpansionMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setTrustRegionExpansionMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set trust region contraction multiplier
     * @param [in] aInput trust region contraction multiplier
    **********************************************************************************/
    void setTrustRegionContractionMultiplier(const ScalarType & aInput)
    {
        mOptimizer->setTrustRegionContractionMultiplier(aInput);
    }

    /******************************************************************************//**
     * @brief Set all upper bounds on particle positions to input scalar
     * @param [in] aInput upper bound
    **********************************************************************************/
    void setUpperBounds(const ScalarType & aInput)
    {
        mOptimizer->setUpperBounds(aInput);
    }

    /******************************************************************************//**
     * @brief Set all lower bounds on particle positions to input scalar
     * @param [in] aInput lower bound
    **********************************************************************************/
    void setLowerBounds(const ScalarType & aInput)
    {
        mOptimizer->setLowerBounds(aInput);
    }

    /******************************************************************************//**
     * @brief Set upper bounds on particle positions
     * @param [in] aInput upper bounds
    **********************************************************************************/
    void setUpperBounds(const Plato::Vector<ScalarType, OrdinalType> & aInput)
    {
        mOptimizer->setUpperBounds(aInput);
    }

    /******************************************************************************//**
     * @brief Set lower bounds on particle positions
     * @param [in] aInput lower bounds
    **********************************************************************************/
    void setLowerBounds(const Plato::Vector<ScalarType, OrdinalType> & aInput)
    {
        mOptimizer->setLowerBounds(aInput);
    }

    /******************************************************************************//**
     * @brief Set custom output interface
     * @param [in] aInput output interface shared pointer
    **********************************************************************************/
    void setCustomOutput(const std::shared_ptr<Plato::CustomOutput<ScalarType,OrdinalType>> & aInput)
    {
        mCustomOutput = aInput;
    }

    /******************************************************************************//**
     * @brief Return current number of iterations
     * @return current number of iterations
    **********************************************************************************/
    OrdinalType getNumIterations() const
    {
        return (mNumIterations);
    }

    /******************************************************************************//**
     * @brief Return number of constraints
     * @return number of constraints
    **********************************************************************************/
    OrdinalType getNumConstraints() const
    {
        return (mStageMng->getNumConstraints());
    }

    /******************************************************************************//**
     * @brief Return current number of augmented Lagrangian function evaluations
     * @return current number of augmented Lagrangian function evaluations
    **********************************************************************************/
    OrdinalType getNumAugLagFuncEvaluations() const
    {
        return (mStageMng->getNumAugLagFuncEvaluations());
    }

    /******************************************************************************//**
     * @brief Return mean of current best augmented Lagrangian function values
     * @return mean of current best augmented Lagrangian function values
    **********************************************************************************/
    ScalarType getMeanCurrentBestAugLagValues() const
    {
        return (mOptimizer->getMeanCurrentBestObjFuncValues());
    }

    /******************************************************************************//**
     * @brief Return standard deviation of current best augmented Lagrangian function values
     * @return standard deviation of current best augmented Lagrangian function values
    **********************************************************************************/
    ScalarType getStdDevCurrentBestAugLagValues() const
    {
        return (mOptimizer->getStdDevCurrentBestObjFuncValues());
    }

    /******************************************************************************//**
     * @brief Return current global best augmented Lagrangian function value
     * @return current global best augmented Lagrangian function value
    **********************************************************************************/
    ScalarType getCurrentGlobalBestAugLagValue() const
    {
        return (mOptimizer->getCurrentGlobalBestObjFuncValue());
    }

    /******************************************************************************//**
     * @brief Return mean of current best values for this constraint
     * @param [in] constraint index
     * @return mean of current best values for this constraint
    **********************************************************************************/
    ScalarType getMeanCurrentBestConstraintValues(const OrdinalType & aIndex) const
    {
        return (mStageMng->getMeanCurrentBestConstraintValues(aIndex));
    }

    /******************************************************************************//**
     * @brief Return standard deviation of current best values for this constraint
     * @param [in] constraint index
     * @return standard deviation of current best values for this constraint
    **********************************************************************************/
    ScalarType getStdDevCurrentBestConstraintValues(const OrdinalType & aIndex) const
    {
        return (mStageMng->getStdDevCurrentBestConstraintValues(aIndex));
    }

    /******************************************************************************//**
     * @brief Return current global best values for this constraint
     * @param [in] constraint index
     * @return current global best values for this constraint
    **********************************************************************************/
    ScalarType getCurrentGlobalBestConstraintValue(const OrdinalType & aIndex) const
    {
        return (mStageMng->getCurrentGlobalBestConstraintValue(aIndex));
    }

    /******************************************************************************//**
     * @brief Get mean current best particle positions
     * @param [out] aInput mean current best particle positions
    **********************************************************************************/
    void getMeanCurrentBestParticlePositions(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = this->getDataMng();
        const Plato::Vector<ScalarType, OrdinalType> & tMeanParticlePositions = tDataMng.getMeanParticlePositions();
        if(aInput.get() == nullptr)
        {
            aInput = tMeanParticlePositions.create();
        }
        assert(aInput->size() == tMeanParticlePositions.size());
        aInput->update(static_cast<ScalarType>(1), tMeanParticlePositions, static_cast<ScalarType>(0));
    }

    /******************************************************************************//**
     * @brief Get standard deviation current best particle positions
     * @param [out] aInput standard deviation current best particle positions
    **********************************************************************************/
    void getStdDevCurrentBestParticlePositions(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = this->getDataMng();
        const Plato::Vector<ScalarType, OrdinalType> & tStdDevParticlePositions = tDataMng.getStdDevParticlePositions();
        if(aInput.get() == nullptr)
        {
            aInput = tStdDevParticlePositions.create();
        }
        assert(aInput->size() == tStdDevParticlePositions.size());
        aInput->update(static_cast<ScalarType>(1), tStdDevParticlePositions, static_cast<ScalarType>(0));
    }

    /******************************************************************************//**
     * @brief Get current global best particle positions
     * @param [out] aInput global best particle positions
    **********************************************************************************/
    void getCurrentGlobalBestParticlePosition(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = this->getDataMng();
        const Plato::Vector<ScalarType, OrdinalType> & tGlobalBestParticlePosition = tDataMng.getGlobalBestParticlePosition();
        if(aInput.get() == nullptr)
        {
            aInput = tGlobalBestParticlePosition.create();
        }
        assert(aInput->size() == tGlobalBestParticlePosition.size());
        aInput->update(static_cast<ScalarType>(1), tGlobalBestParticlePosition, static_cast<ScalarType>(0));
    }

    /******************************************************************************//**
     * @brief Get mean current best constraint values
     * @param [out] aInput mean current best constraint values
    **********************************************************************************/
    void getMeanCurrentBestConstraintValues(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::Vector<ScalarType, OrdinalType> & tMeanConstraintValues = mStageMng->getMeanCurrentBestConstraintValues();
        if(aInput.get() == nullptr)
        {
            aInput = tMeanConstraintValues.create();
        }
        assert(aInput->size() == tMeanConstraintValues.size());
        aInput->update(static_cast<ScalarType>(1), tMeanConstraintValues, static_cast<ScalarType>(0));
    }

    /******************************************************************************//**
     * @brief Get standard deviation current best constraint values
     * @param [out] aInput standard deviation current best constraint values
    **********************************************************************************/
    void getStdDevCurrentBestConstraintValues(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::Vector<ScalarType, OrdinalType> & tStdDevConstraintValues = mStageMng->getStdDevCurrentBestConstraintValues();
        if(aInput.get() == nullptr)
        {
            aInput = tStdDevConstraintValues.create();
        }
        assert(aInput->size() == tStdDevConstraintValues.size());
        aInput->update(static_cast<ScalarType>(1), tStdDevConstraintValues, static_cast<ScalarType>(0));
    }

    /******************************************************************************//**
     * @brief Get current global best constraint values
     * @param [out] aInput current global best constraint values
    **********************************************************************************/
    void getCurrentGlobalBestConstraintValues(std::shared_ptr<Plato::Vector<ScalarType, OrdinalType>> & aInput) const
    {
        const Plato::Vector<ScalarType, OrdinalType> & tGlobalBestConstraintValues = mStageMng->getCurrentGlobalBestConstraintValues();
        if(aInput.get() == nullptr)
        {
            aInput = tGlobalBestConstraintValues.create();
        }
        assert(aInput->size() == tGlobalBestConstraintValues.size());
        aInput->update(static_cast<ScalarType>(1), tGlobalBestConstraintValues, static_cast<ScalarType>(0));
    }


    /******************************************************************************//**
     * @brief Return stopping criterion
     * @return stopping criterion string
    **********************************************************************************/
    std::string getStoppingCriterion() const
    {
        std::string tReason;
        Plato::pso::get_stop_criterion(mStopCriterion, tReason);
        return (tReason);
    }


    /******************************************************************************//**
     * @brief Return PSO stage manager
     * @return non-const reference to PSO stage manager
    **********************************************************************************/
    Plato::ParticleSwarmStageMng<ScalarType, OrdinalType> & getStageMng()
    {
        return (*mStageMng);
    }


    /******************************************************************************//**
     * @brief Return PSO data manager
     * @return const reference to PSO data manager
    **********************************************************************************/
    const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & getDataMng() const
    {
        return (mOptimizer->getDataMng());
    }

    /******************************************************************************//**
     * @brief Solve constrained particle swarm optimization problem
    **********************************************************************************/
    void solve()
    {
        assert(mStageMng.get() != nullptr);
        assert(mOptimizer.get() != nullptr);

        mNumIterations = 0;
        this->openOutputFiles();
        this->initialize();

        while(1)
        {
            mNumIterations++;
            mOptimizer->solve(mAlgoOutputStream);
            mStageMng->computeCriteriaStatistics();

            this->outputDiagnostics();
            if(this->checkStoppingCriteria())
            {
                mOptimizer->computeCurrentBestParticlesStatistics();
                this->outputStoppingCriterion();
                this->closeOutputFiles();
                break;
            }

            mStageMng->updatePenaltyMultipliers();
            mStageMng->updateLagrangeMultipliers();
            mStageMng->restart();
        }
    }

private:
    /******************************************************************************//**
     * @brief Initialize and allocate class member data
    **********************************************************************************/
    void initialize()
    {
        mOptimizer->initialize();
        const OrdinalType tNumConstraints = mStageMng->getNumConstraints();
        mOptimizer->setNumConstraints(tNumConstraints);
    }

    /******************************************************************************//**
     * @brief Check stopping criteria
     * @return flag (true = stop : false = continue)
    **********************************************************************************/
    bool checkStoppingCriteria()
    {
        bool tStop = false;
        const ScalarType tTrustRegionMultiplier = mOptimizer->getTrustRegionMultiplier();
        const ScalarType tMeanCurrentBestObjFuncValue = mOptimizer->getMeanCurrentBestObjFuncValues();
        const ScalarType tCurrentGlobalBestAugLagFuncValue = mOptimizer->getCurrentGlobalBestObjFuncValue();
        const ScalarType tStdDevCurrentBestAugLagFuncValue = mOptimizer->getStdDevCurrentBestObjFuncValues();

        if(mNumIterations >= mMaxNumAugLagOuterIterations)
        {
            tStop = true;
            mStopCriterion = Plato::particle_swarm::MAX_NUMBER_ITERATIONS;
        }
        else if(tCurrentGlobalBestAugLagFuncValue < mGlobalBestAugLagFuncTolerance)
        {
            tStop = true;
            mStopCriterion = Plato::particle_swarm::TRUE_OBJECTIVE_TOLERANCE;
        }
        else if(tMeanCurrentBestObjFuncValue < mMeanBestAugLagFuncTolerance)
        {
            tStop = true;
            mStopCriterion = Plato::particle_swarm::MEAN_OBJECTIVE_TOLERANCE;
        }
        else if(tStdDevCurrentBestAugLagFuncValue < mStdDevBestAugLagFuncTolerance && mStdDevStoppingTolActive)
        {
            tStop = true;
            mStopCriterion = Plato::particle_swarm::STDDEV_OBJECTIVE_TOLERANCE;
        }
        else if(tTrustRegionMultiplier < mTrustRegionMultiplierTolerance)
        {
            tStop = true;
            mStopCriterion = Plato::particle_swarm::TRUST_REGION_MULTIPLIER_TOLERANCE;
        }

        return (tStop);
    }

    /******************************************************************************//**
     * @brief Open diagnostic files
    **********************************************************************************/
    void openOutputFiles()
    {
        this->openAlgoOutputFile();
        this->openParticleOutputFiles();
    }

    /******************************************************************************//**
     * @brief Open ALPSO algorithm diagnostics file
    **********************************************************************************/
    void openAlgoOutputFile()
    {
        if(mAlgorithmDiagnostics == true)
        {
            const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
            const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mAlgoOutputStream.open("plato_alpso_algorithm_diagnostics.txt");
                Plato::pso::print_alpso_diagnostics_header(mOutputData, mAlgoOutputStream);
            }
        }
    }

    /******************************************************************************//**
     * @brief Open ALPSO particle history files (i.e. particle sets diagnostics files)
    **********************************************************************************/
    void openParticleOutputFiles()
    {
        if(mParticleDiagnostics == true)
        {
            const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
            const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mBestParticlesStream.open("plato_alpso_best_particles.txt");
                Plato::pso::print_particle_data_header(mBestParticlesStream);
                mTrialParticlesStream.open("plato_alpso_trial_particles.txt");
                Plato::pso::print_particle_data_header(mTrialParticlesStream);
                mGlobalBestParticlesStream.open("plato_alpso_global_best_particle.txt");
                Plato::pso::print_global_best_particle_data_header(mGlobalBestParticlesStream);
            }
        }
    }

    /******************************************************************************//**
     * @brief Close diagnostic files
    **********************************************************************************/
    void closeOutputFiles()
    {
        this->closeAlgoOutputFiles();
        this->closeParticleOutputFiles();
    }

    /******************************************************************************//**
     * @brief Close ALPSO algorithm diagnostics file
    **********************************************************************************/
    void closeAlgoOutputFiles()
    {
        if(mAlgorithmDiagnostics == true)
        {
            const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
            const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mAlgoOutputStream.close();
            }
        }
    }

    /******************************************************************************//**
     * @brief Close ALPSO particle history/diagnostics files
    **********************************************************************************/
    void closeParticleOutputFiles()
    {
        if(mParticleDiagnostics == true)
        {
            const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
            const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                mBestParticlesStream.close();
                mTrialParticlesStream.close();
                mGlobalBestParticlesStream.close();
            }
        }
    }

    /******************************************************************************//**
     * @brief Print stopping criterion into diagnostics file.
    **********************************************************************************/
    void outputStoppingCriterion()
    {
        if(mAlgorithmDiagnostics == true)
        {
            const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
            const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
            if(tMyCommWrapper.myProcID() == 0)
            {
                std::string tReason;
                Plato::pso::get_stop_criterion(mStopCriterion, tReason);
                mAlgoOutputStream << tReason.c_str();
            }
        }
    }

    /******************************************************************************//**
     * @brief Print diagnostics for ALPSO algorithm
    **********************************************************************************/
    void outputDiagnostics()
    {
        this->outputAlgoDiagnostics();
        this->outputBestParticleDiagnostics();
        this->outputTrialParticleDiagnostics();
        this->outputGlobalBestParticleDiagnostics();
    }

    /******************************************************************************//**
     * @brief Output diagnostics for ALPSO algorithm
    **********************************************************************************/
    void outputAlgoDiagnostics()
    {
        if(mAlgorithmDiagnostics == false)
        {
            return;
        }

        mCustomOutput->output();
        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
        const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
        if(tMyCommWrapper.myProcID() == 0)
        {
            this->cacheObjFuncOutputData();
            this->cacheConstraintOutputData();
            Plato::pso::print_alpso_outer_diagnostics(mOutputData, mAlgoOutputStream);
        }
    }

    /******************************************************************************//**
     * @brief Output diagnostics for best particle set
    **********************************************************************************/
    void outputBestParticleDiagnostics()
    {
        if(mParticleDiagnostics == false)
        {
            return;
        }

        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
        const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
        if(tMyCommWrapper.myProcID() == 0)
        {
            Plato::pso::print_particle_data(mNumIterations,
                                            tDataMng.getCurrentBestObjFuncValues(),
                                            tDataMng.getBestParticlePositions(),
                                            mBestParticlesStream);
        }
    }

    /******************************************************************************//**
     * @brief Output diagnostics for trial particle set
    **********************************************************************************/
    void outputTrialParticleDiagnostics()
    {
        if(mParticleDiagnostics == false)
        {
            return;
        }

        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
        const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
        if(tMyCommWrapper.myProcID() == 0)
        {
            Plato::pso::print_particle_data(mNumIterations,
                                            tDataMng.getCurrentObjFuncValues(),
                                            tDataMng.getCurrentParticles(),
                                            mTrialParticlesStream);
        }
    }

    /******************************************************************************//**
     * @brief Output diagnostics for global best particle
    **********************************************************************************/
    void outputGlobalBestParticleDiagnostics()
    {
        if(mParticleDiagnostics == false)
        {
            return;
        }

        const Plato::ParticleSwarmDataMng<ScalarType, OrdinalType> & tDataMng = mOptimizer->getDataMng();
        const Plato::CommWrapper& tMyCommWrapper = tDataMng.getCommWrapper();
        if(tMyCommWrapper.myProcID() == 0)
        {
            Plato::pso::print_global_best_particle_data(mNumIterations,
                                                        tDataMng.getCurrentGlobalBestParticleIndex(),
                                                        mOptimizer->getCurrentGlobalBestObjFuncValue(),
                                                        tDataMng.getGlobalBestParticlePosition(),
                                                        mGlobalBestParticlesStream);
        }
    }

    /******************************************************************************//**
     * @brief Cache diagnostic data for the objective function
    **********************************************************************************/
    void cacheObjFuncOutputData()
    {
        mOutputData.mNumIter = mNumIterations;
        mOutputData.mAugLagFuncCount = mStageMng->getNumAugLagFuncEvaluations();
        mOutputData.mMeanCurrentBestAugLagValues = mOptimizer->getMeanCurrentBestObjFuncValues();
        mOutputData.mCurrentGlobalBestAugLagValue = mOptimizer->getCurrentGlobalBestObjFuncValue();
        mOutputData.mStdDevCurrentBestAugLagValues = mOptimizer->getStdDevCurrentBestObjFuncValues();

        mOutputData.mMeanCurrentBestObjFuncValues = mStageMng->getMeanCurrentBestObjFuncValues();
        mOutputData.mCurrentGlobalBestObjFuncValue = mStageMng->getCurrentGlobalBestObjFuncValue();
        mOutputData.mStdDevCurrentBestObjFuncValues = mStageMng->getStdDevCurrentBestObjFuncValues();
    }

    /******************************************************************************//**
     * @brief Cache diagnostic data for all constraints
    **********************************************************************************/
    void cacheConstraintOutputData()
    {
        for(OrdinalType tIndex = 0; tIndex < mOutputData.mNumConstraints; tIndex++)
        {
            mOutputData.mMeanCurrentBestConstraintValues[tIndex] = mStageMng->getMeanCurrentBestConstraintValues(tIndex);
            mOutputData.mStdDevCurrentBestConstraintValues[tIndex] = mStageMng->getStdDevCurrentBestConstraintValues(tIndex);
            mOutputData.mCurrentGlobalBestConstraintValues[tIndex] = mStageMng->getCurrentGlobalBestConstraintValue(tIndex);

            mOutputData.mMeanCurrentPenaltyMultipliers[tIndex] = mStageMng->getMeanCurrentPenaltyMultipliers(tIndex);
            mOutputData.mStdDevCurrentPenaltyMultipliers[tIndex] = mStageMng->getStdDevCurrentPenaltyMultipliers(tIndex);
            mOutputData.mMeanCurrentLagrangeMultipliers[tIndex] = mStageMng->getMeanCurrentLagrangeMultipliers(tIndex);
            mOutputData.mStdDevCurrentLagrangeMultipliers[tIndex] = mStageMng->getStdDevCurrentLagrangeMultipliers(tIndex);
        }
    }

private:
    bool mParticleDiagnostics; /*!< flag - print particle diagnostics (default=false) */
    bool mAlgorithmDiagnostics; /*!< flag - print algorithm diagnostics (default = false) */
    bool mStdDevStoppingTolActive; /*!< activate standard deviation stopping tolerance (default = true) */

    std::ofstream mAlgoOutputStream; /*!< output stream for algorithm's diagnostics */
    std::ofstream mBestParticlesStream; /*!< output stream for best particles */
    std::ofstream mTrialParticlesStream; /*!< output stream for trial particles */
    std::ofstream mGlobalBestParticlesStream; /*!< output stream for global best particles */

    OrdinalType mNumIterations; /*!< current number of iterations */
    OrdinalType mMaxNumAugLagOuterIterations; /*!< maximum number of outer iterations */

    ScalarType mMeanBestAugLagFuncTolerance; /*!< stopping tolerance on the mean of the best augmented Lagrangian function values */
    ScalarType mStdDevBestAugLagFuncTolerance; /*!< stopping tolerance on the standard deviation of the best augmented Lagrangian function values */
    ScalarType mGlobalBestAugLagFuncTolerance; /*!< stopping tolerance on global best augmented Lagrangian function value */
    ScalarType mTrustRegionMultiplierTolerance; /*!< stopping tolerance on the trust region multiplier */

    Plato::particle_swarm::stop_t mStopCriterion; /*!< stopping criterion enum */
    Plato::DiagnosticsALPSO<ScalarType, OrdinalType> mOutputData; /*!< ALPSO algorithm output/diagnostics data structure */

    std::shared_ptr<Plato::CustomOutput<ScalarType,OrdinalType>> mCustomOutput;  /*!< custom output interface */
    std::shared_ptr<Plato::ParticleSwarmStageMngALPSO<ScalarType, OrdinalType>> mStageMng; /*!< stage manager interface for ALPSO */
    std::shared_ptr<Plato::ParticleSwarmAlgorithmBCPSO<ScalarType, OrdinalType>> mOptimizer; /*!< interface to BCPSO algorithm */

private:
    ParticleSwarmAlgorithmALPSO(const Plato::ParticleSwarmAlgorithmALPSO<ScalarType, OrdinalType>&);
    Plato::ParticleSwarmAlgorithmALPSO<ScalarType, OrdinalType> & operator=(const Plato::ParticleSwarmAlgorithmALPSO<ScalarType, OrdinalType>&);
};
// class ParticleSwarmAlgorithmALPSO

} // namespace Plato
