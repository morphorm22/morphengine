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
 * Plato_Interface.cpp
 *
 *  Created on: March 20, 2017
 *
 */

#include <limits>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <fstream>

#include "Plato_Interface.hpp"
#include "Plato_InputData.hpp"
#include "Plato_SharedField.hpp"
#include "Plato_SharedValue.hpp"
#include "Plato_Application.hpp"
#include "Plato_Performer.hpp"
#include "Plato_Operation.hpp"
#include "Plato_Operation.hpp"
#include "Plato_Stage.hpp"
#include "Plato_Exceptions.hpp"
#include "Plato_Parser.hpp"
#include "Plato_SharedDataInfo.hpp"
#include "Plato_StageInputDataMng.hpp"

namespace Plato
{

namespace{
/// @return The local program's communicator id from the environment.
/// This value (PLATO_PERFORMER_ID) is specified as an argument to mpirun.
/// An invalid value of -1 is returned if the environment variable doesn't exist.
int performerID()
{
    const char* const tPerfChar = getenv("PLATO_PERFORMER_ID");
    if(tPerfChar)
    {
        return atoi(tPerfChar);
    }
    else
    {
        return -1;
    }
}
}

/******************************************************************************/
Interface::Interface(MPI_Comm aGlobalComm) :
/******************************************************************************/
        mPerformerID(performerID()),
        mInputData(inputDataFromPugiParsedFile(getenv("PLATO_INTERFACE_FILE"))),
        mGlobalComm(aGlobalComm)
{
    createPerformers();
    initializeConsole();
}

/******************************************************************************/
Interface::Interface(const XMLFileName& aFileName, const XMLNodeName& aNodeName, MPI_Comm aGlobalComm) :
        mPerformerID(performerID()),
        mInputData(inputDataFromPugiParsedFile(getenv("PLATO_INTERFACE_FILE"))),
        mGlobalComm(aGlobalComm)
/******************************************************************************/
{
    Plato::loadFromXML(*this, aFileName, aNodeName);
    initializePerformerMPI();
    initializeConsole();
}

/******************************************************************************/
Interface::Interface(const int & aCommID, const std::string & aXML_String, MPI_Comm aGlobalComm) :
        mLocalCommID(aCommID),
        mInputData(inputDataFromPugiParsedFile(getenv("PLATO_INTERFACE_FILE"))),
        mGlobalComm(aGlobalComm)
/******************************************************************************/
{
    // The local program's communicator id (in argument aCommID) is specified as an 
    // argument to mpirun.
    // TODO: Add validation, not sure what valid is here
    // if(comm_id is valid) 
    //else
    // throw

    createPerformers();
    initializeConsole();
}

/******************************************************************************/
void Interface::getLocalComm(MPI_Comm& aLocalComm)
/******************************************************************************/
{
    aLocalComm = mLocalComm;
}

/******************************************************************************/
Plato::InputData Interface::getInputData() const
/******************************************************************************/
{
    return mInputData;
}

/******************************************************************************/
int Interface::getStageIndex(std::string aStageName) const
/******************************************************************************/
{
    for(size_t tIndex = 0; tIndex < mStages.size(); ++tIndex)
    {
        if(mStages[tIndex]->getName() == aStageName)
        {
            return tIndex;
        }
    }
    return -1;
}

/******************************************************************************/
void Interface::broadcastStageIndex(int & aStageIndex)
/******************************************************************************/
{
    MPI_Bcast(&aStageIndex, 1, MPI_INT, 0, mGlobalComm);

    if(aStageIndex == INVALID_STAGE)
    {
        std::stringstream tMsg;
        tMsg << "\n\n ********** PLATO ERROR: Interface::broadcastStageIndex: Invalid stage requested.\n\n";
        Plato::ParsingException tParsingException(tMsg.str());
        registerException(tParsingException);
    } else
    if(aStageIndex == TERMINATE_STAGE)
    {
        std::stringstream tMsg;
        tMsg << "\n\n INFO: Interface::broadcastStageIndex: Terminate stage requested.  Exiting.\n\n";
        Plato::TerminateSignal tTerminateSignal(tMsg.str());
        registerException(tTerminateSignal);
    }
    this->handleExceptions();
}

/******************************************************************************/
Plato::Stage*
Interface::getStage(std::string aStageName)
/******************************************************************************/
{
    // check for control file
    std::ifstream tControlFile;
    tControlFile.open("plato.control");
    if(tControlFile)
    {
        Plato::Parser* parser = new Plato::PugiParser();
        auto tControlData = parser->parseFile("plato.control");
        delete parser;

        auto tTerminate = Plato::Get::Bool(tControlData, "Terminate", false);

        if(tTerminate)
        {
            aStageName = "Terminate";
        }
    }

    // broadcast the index of the next stage
    int tStageIndex;
    if(aStageName == "Terminate")
        tStageIndex = TERMINATE_STAGE;
    else
        tStageIndex = getStageIndex(aStageName);

    broadcastStageIndex(tStageIndex);

    if(tStageIndex >= 0)
    {
        return mStages[tStageIndex];
    }
    else
    {
        mIsDone = true;
        return nullptr;
    }
}

/******************************************************************************/
Plato::Stage*
Interface::getStage()
/******************************************************************************/
{
    // broadcast the index of the next stage
    int tStageIndex;

    broadcastStageIndex(tStageIndex);

    if(tStageIndex >= 0)
    {
        return mStages[tStageIndex];
    }
    else
    {
        mIsDone = true;
        return nullptr;
    }
}

/******************************************************************************/
void Interface::run()
/******************************************************************************/
{
  // Note: this method is a stub and is a reminder that the try loop
  // in the main in Plato_Main.cpp should really be encapsulated and
  // be part of the interface. However doing so pulls all of the
  // optimizers into the interface library. It also requires the
  // driver library be included when building Analyze_MPMD. Future
  // works is to refactor.

  /*
    // This handleException matches the one in Interface::perform().
    this->handleExceptions();

    // There should be at least one driver block but there can
    // be more. These additional driver blocks can be in serial
    // or nested. The while loop coupled with factory processes
    // driver blocks that are serial. Nested driver blocks
    // are processed recursively via the EngineObjective.
    Plato::DriverFactory<double> tDriverFactory;
    Plato::DriverInterface<double>* tDriver = nullptr;

    // Note: When first called, the factory will look for the
    // first driver block. Subsequent calls will look for the
    // next driver block if it exists.
    while((tDriver =
           tDriverFactory.create(this, mLocalComm)) != nullptr)
    {
        tDriver->run();

        // If the last driver do any final stages before deleting
        // it. This call goes to the driver and then back to the
        // interface finalize with possibly a stage to compute.
        if( tDriver->lastDriver() )
        {
            tDriver->finalize();
        }

        delete tDriver;
    }
  */
}

/******************************************************************************/
void Interface::perform()
/******************************************************************************/
{
    // This handleException matches the one in Plato_Main.cpp main().
    this->handleExceptions();

    while(this->isDone() == false)
    {
        // Performers 'hang' here until a new stage is established
        Plato::Stage* tStage = this->getStage();
        // 'Terminate' stage is nullptr
        if(tStage == nullptr)
        {
            continue;
        }

        this->perform(tStage);
    }

    mPerformer->finalize();
}

/******************************************************************************/
void Interface::reinitializePerformer()
/******************************************************************************/
{
    mPerformer->getApplication()->reinitialize();
}

/******************************************************************************/
void Interface::perform(Plato::Stage* aStage)
/******************************************************************************/
{
    // Console::Status("Perform Stage: (" + mPerformer->myName() + ") " + aStage->getName());

    // Intercept this stage as it is an internal stage. That is the
    // user does not need to define it.
    if( aStage->getName() == "Update Shared Data" )
    {
        Console::Status("Perform Stage: (" + mPerformer->myName() + ") " + aStage->getName());

        this->createSharedData( mPerformer->getApplication() );

        // After creating the shared data all of the stages and their
        // operations need to be updated so to have the new links to
        // the shared data.
        this->updateStages();

        this->reinitializePerformer();
    }
    else
    {
        // transmits input data
        //
        aStage->begin();

        // any local operations?
        //
        Plato::Operation* tOperation = aStage->getNextOperation();
        while(tOperation)
        {
            // Console::Status("Perform Operation: (" + mPerformer->myName() + ") " + tOperation->getOperationName());
            tOperation->sendInput();

            // copy data from Plato::SharedData buffers to hostedCode data containers
            //
            std::vector<std::string> tOperationInputDataNames = tOperation->getInputDataNames();
            for(std::string tName : tOperationInputDataNames)
            {
                try
                {
                    tOperation->importData(tName, mDataLayer->getSharedData(tName));
                }
                catch(...)
                {
                    mExceptionHandler->Catch();
                }
                this->handleExceptions();
            }

            try
            {
                tOperation->compute();
            }
            catch(...)
            {
                mExceptionHandler->Catch();
            }
            this->handleExceptions();

            // copy data from hostedCode data containers to Plato::SharedData buffers
            //
            std::vector<std::string> tOperationOutputDataNames = tOperation->getOutputDataNames();
            for(std::string tName : tOperationOutputDataNames)
            {
                try
                {
                    tOperation->exportData(tName, mDataLayer->getSharedData(tName));
                }
                catch(...)
                {
                    mExceptionHandler->Catch();
                }
                this->handleExceptions();
            }

            tOperation->sendOutput();

            tOperation = aStage->getNextOperation();
        }

        // transmits output data
        //
        aStage->end();
    }
}

/******************************************************************************/
void Interface::finalize( std::string aStageName )
/******************************************************************************/
{
    // Typically a stage for writing the solution to the output file.
    if(aStageName.empty() == false)
    {
        Teuchos::ParameterList tParameterList;
        this->compute(aStageName, tParameterList);
    }

    // At this point all drivers have completed so terminate.
    this->getStage("Terminate");
}

/******************************************************************************/
void Interface::compute(const std::vector<std::string> & aStageNames, Teuchos::ParameterList& aArguments)
/******************************************************************************/
{
    for(const std::string & tStageName : aStageNames)
    {
        this->compute(tStageName, aArguments);
    }
}

/******************************************************************************/
void Interface::compute(const std::string & aStageName, Teuchos::ParameterList& aArguments)
/******************************************************************************/
{
    // Console::Status("Compute Stage: (" + mPerformer->myName() + ") " + aStageName);

    // Find the requested stage
    Plato::Stage* tStage = getStage(aStageName);

    if( tStage == nullptr )
    {
        std::stringstream tMsg;
        tMsg << "\n\n ********** PLATO ERROR: Interface::compute: Invalid stage requested: " << aStageName << "\n\n";
        Plato::ParsingException tParsingException(tMsg.str());
        registerException(tParsingException);
    }

    // Unpack input arguments into Plato::SharedData
    //
    std::vector<std::string> tStageInputDataNames = tStage->getInputDataNames();
    for(std::string tName : tStageInputDataNames)
    {
        exportData(aArguments.get<double*>(tName), mDataLayer->getSharedData(tName));
    }

    this->perform(tStage);

    // Unpack output arguments from Plato::SharedData
    //
    std::vector<std::string> tStageOutputDataNames = tStage->getOutputDataNames();
    for(std::string tName : tStageOutputDataNames)
    {
        importData(aArguments.get<double*>(tName), mDataLayer->getSharedData(tName));
    }
}

/******************************************************************************/
void Interface::exportData(double* aFrom, Plato::SharedData* aTo)
/******************************************************************************/
{
    int tMyLength = aTo->size();
    std::vector<double> tExportData(tMyLength);
    std::copy(aFrom, aFrom + tMyLength, tExportData.begin());
    aTo->setData(tExportData);
}

/******************************************************************************/
void Interface::importData(double* aTo, Plato::SharedData* aFrom)
/******************************************************************************/
{
    int tMyLength = aFrom->size();
    std::vector<double> tImportData(tMyLength);
    aFrom->getData(tImportData);
    std::copy(tImportData.begin(), tImportData.end(), aTo);
}

/******************************************************************************/
void Interface::registerApplication(Plato::Application* aApplication)
/******************************************************************************/
{
    checkAndSetApplication(aApplication);
    tryFCatchInterfaceExceptions([aApplication](){aApplication->initialize();});
    tryFCatchInterfaceExceptions([this, aApplication](){createSharedData(aApplication);});
    tryFCatchInterfaceExceptions([this](){createStages();});
}

/******************************************************************************/
void Interface::registerApplicationOnlyInitializeMPI(Application* aApplication)
/******************************************************************************/
{
    checkAndSetApplication(aApplication);
    setPerformerOnStages();
    initializeSharedDataMPI(aApplication);
}

/******************************************************************************/
void Interface::createStages()
/******************************************************************************/
{
    auto tStages = mInputData.getByName<Plato::InputData>("Stage");
    for(auto tStageNode=tStages.begin(); tStageNode!=tStages.end(); ++tStageNode)
    {
        Plato::StageInputDataMng tStageInputDataMng;
        Plato::Parse::parseStageData(*tStageNode, tStageInputDataMng);
        Plato::Stage* tNewStage = new Plato::Stage(tStageInputDataMng, mPerformer, mDataLayer->getSharedData());
        mStages.push_back(tNewStage);
    }

    // Add the internal stages. That is stages that the user does not
    // need to define.
    {
        Plato::StageInputDataMng tStageInputDataMng;
        tStageInputDataMng.add("Update Shared Data");
        Plato::Stage* tNewStage = new Plato::Stage(tStageInputDataMng, mPerformer, mDataLayer->getSharedData());
        mStages.push_back(tNewStage);
    }
}

/******************************************************************************/
void Interface::updateStages()
/******************************************************************************/
{
    // Serialization TODO: I don't think this will be correctly handled without an interface file

    // After creating the shared data all of the stages and their
    // operations need to be updated so to have the new links to
    // the shared data.
    auto tStages = mInputData.getByName<Plato::InputData>("Stage");

    for(auto tStageNode=tStages.begin(); tStageNode!=tStages.end(); ++tStageNode)
    {
        Plato::StageInputDataMng tStageInputDataMng;
        Plato::Parse::parseStageData(*tStageNode, tStageInputDataMng);

        Plato::Stage* tStage =
          mStages[ getStageIndex( tStageInputDataMng.getStageName() ) ];

        tStage->update(tStageInputDataMng, mPerformer, mDataLayer->getSharedData());
    }

    // Update the internal stages. That is stages that the user does not
    // need to define.
    {
        // This update is really a no-op as there is no shared data.
        // The update is done though for consistancy (and as an
        // example for future internal stages).

        Plato::StageInputDataMng tStageInputDataMng;
        tStageInputDataMng.add("Update Shared Data");
        Plato::Stage* tStage =
          mStages[ getStageIndex( tStageInputDataMng.getStageName() ) ];

        tStage->update(tStageInputDataMng, mPerformer, mDataLayer->getSharedData());
    }
}

/******************************************************************************/
void Interface::createPerformers()
/******************************************************************************/
{
    for( auto tNode : mInputData.getByName<Plato::InputData>("Performer") )
    {
        int tLocalPerformerID = Plato::Get::Int(tNode, "PerformerID", std::numeric_limits<int>::min());
        std::vector<std::string> tPerformerNames;
        for(const auto& tName : tNode.getByName<std::string>("Name"))
        {
            tPerformerNames.push_back(tName);
        }
        const std::string& tPerformerCode = Plato::Get::String(tNode, "Code");
        // C++20 use designated initializers:
        mAllPerformersInfo.push_back({tPerformerNames, tPerformerCode, tLocalPerformerID});
    }
    initializePerformerMPI();
}

/******************************************************************************/
void Interface::createSharedData(Plato::Application* const aApplication)
/******************************************************************************/
{
    for( auto tNode : mInputData.getByName<Plato::InputData>("SharedData") )
    {
        // Use designated initializers in C++20
        mAllSharedDataInfo.push_back({
            Plato::Get::String(tNode, "Name"),
            Plato::Get::String(tNode, "Layout"),
            tNode.size<std::string>("Size") ? Plato::Get::Int(tNode, "Size") : 1,
            Plato::Get::Bool(tNode, "Dynamic", false),
            tNode.getByName<std::string>("OwnerName"),
            tNode.getByName<std::string>("UserName")
        });
    }
    SharedDataInfo tSharedDataInfo;
    CommunicationData tCommunicationData;
    getSharedDataAndCommunicationInfo(aApplication, tSharedDataInfo, tCommunicationData);

    if(mDataLayer)
    {
        delete mDataLayer;
    }
    mDataLayer = new Plato::DataLayer(tSharedDataInfo, tCommunicationData);
}

/******************************************************************************/
void Interface::getSharedDataAndCommunicationInfo(Application* const aApplication, 
        SharedDataInfo& aSharedDataInfo,
        CommunicationData& aCommunicationData) const
/******************************************************************************/
{
    aCommunicationData.mLocalComm = mLocalComm;
    aCommunicationData.mInterComm = mGlobalComm;
    aCommunicationData.mLocalCommName = mLocalPerformerName;
    for(const auto& tInfo : mAllSharedDataInfo)
    {
        aSharedDataInfo.setSharedDataMap(tInfo.mProviderNames, tInfo.mReceiverNames);

        Plato::communication::broadcast_t tMyBroadcast =
                Plato::getBroadcastType(aCommunicationData.mLocalCommName, tInfo.mProviderNames, tInfo.mReceiverNames);
        aSharedDataInfo.setMyBroadcast(tMyBroadcast);

        aSharedDataInfo.setSharedDataSize(tInfo.mName, tInfo.mSize);
        aSharedDataInfo.setSharedDataDynamic(tInfo.mName, tInfo.mIsDynamic);

        std::string tLayoutUppercase = tInfo.mLayout;
        Parse::toUppercase(tLayoutUppercase);
        aSharedDataInfo.setSharedDataIdentifiers(tInfo.mName, tLayoutUppercase);
    }

    this->exportGraph(aSharedDataInfo, aApplication, aCommunicationData);
}

/******************************************************************************/
void Interface::initializeSharedDataMPI(Application* const aApplication)
/******************************************************************************/
{
    SharedDataInfo tSharedDataInfo;
    CommunicationData tCommunicationData;
    getSharedDataAndCommunicationInfo(aApplication, tSharedDataInfo, tCommunicationData);
    mDataLayer->initializeMPI(tCommunicationData);
}

/******************************************************************************/
void Interface::initializePerformerMPI()
/******************************************************************************/
{
    int tMyRank, tNumGlobalRanks;
    MPI_Comm_rank(mGlobalComm, &tMyRank);
    MPI_Comm_size(mGlobalComm, &tNumGlobalRanks);

    std::vector<int> tPerfIDs;
    std::map<int,int> tPerfCommSize;
    for(const auto& tPerformer : mAllPerformersInfo)
    {
        // is a PerformerID specified?  If not, error out.
        //
        int tLocalPerformerID = tPerformer.mId;
        if( tLocalPerformerID == std::numeric_limits<int>::min() )
        {
            if( tMyRank == 0 )
            {
                std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
                std::cout << "  Each Performer definition must include a 'PerformerID'." << std::endl;
                std::cout << " -----------------------------------------------------------------------------" << std::endl;
            }
            throw 1;
        }

        // is the PerformerID already used? If so, error out.
        //
        if( std::count( tPerfIDs.begin(), tPerfIDs.end(), tLocalPerformerID ) )
        {
            if( tMyRank == 0 )
            {
                std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
                std::cout << "  Duplicate PerformerID's.  Each performer must have a unique PerformerID." << std::endl;
                std::cout << " -----------------------------------------------------------------------------" << std::endl;
            }
            throw 1;
        }
        else
        {
            tPerfIDs.push_back(tLocalPerformerID);
        }


        // Are any PerformerIDs specified in the interface definition
        // that weren't defined on the mpi command line?
        //
        int tMyPerformerSpec = (tLocalPerformerID == mPerformerID) ? 1 : 0;
        int tNumRanksThisID = 0;
        MPI_Allreduce(&tMyPerformerSpec, &tNumRanksThisID, 1, MPI_INT, MPI_SUM, mGlobalComm);
        if( tNumRanksThisID == 0 ){
            if( tMyRank == 0 )
            {
                std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
                std::cout << "  A Performer spec is defined for which no PerformerID is given on the mpi command line." << std::endl;
                std::cout << " -----------------------------------------------------------------------------" << std::endl;
            }
            throw 1;
        }
        else
        {
           tPerfCommSize[tLocalPerformerID] = tNumRanksThisID;
        }
    }

    // Is there a Performer spec for my local Performer ID?
    //
    int tErrorNoSpec = ( std::count( tPerfIDs.begin(), tPerfIDs.end(), mPerformerID ) == 0 ) ? 1 : 0;
    int tErrorNoSpecGlobal = 0;
    MPI_Allreduce(&tErrorNoSpec, &tErrorNoSpecGlobal, 1, MPI_INT, MPI_MAX, mGlobalComm);
    if( tErrorNoSpecGlobal ){
        if( tMyRank == 0 )
        {
            std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
            std::cout << "  A Performer spec must be provided for each PerformerID defined on the mpi command line." << std::endl;
            std::cout << " -----------------------------------------------------------------------------" << std::endl;
        }
        throw 1;
    }

    // If the Performer spec has N names defined then the allocated
    // ranks on that PerformerID are broken into N local comms.  To
    // avoid any semantics of how ranks are assigned, manually color
    // the local comms before splitting.
    //
    std::vector<int> tPerformerIDs(tNumGlobalRanks);
    MPI_Allgather(&mPerformerID, 1, MPI_INT, tPerformerIDs.data(), 1, MPI_INT, mGlobalComm);

    int tCommIndex = 0;
    for(const auto& tPerformer : mAllPerformersInfo)
    {
        int tLocalPerformerID = tPerformer.mId;
        int tNumRanksThisID = tPerfCommSize[tLocalPerformerID];
        const auto& tPerformerNames = tPerformer.mNames;
        int tNumCommsThisID = tPerformerNames.size();
        int tLocalPerformerCommSize = tNumRanksThisID / tNumCommsThisID;

        // Does the number of Comms partition the ranks for this
        // PerformerID without a remainder?
        //
        int tErrorUneven = ( tNumCommsThisID * tLocalPerformerCommSize == tNumRanksThisID ) ? 0 : 1;
        if( tErrorUneven ){
            if( tMyRank == 0 )
            {
                    std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
                    std::cout << "  Error creating performer with ID=" << tLocalPerformerID << "." << std::endl;
                    std::cout << "  Attempted to spread N=" << tNumRanksThisID
                              << " processes over M=" << tNumCommsThisID << " comms, but N % M != 0." << std::endl;
                    std::cout << "  Change N to a whole number multiple of M and try again." << std::endl;
                    std::cout << " -----------------------------------------------------------------------------" << std::endl;
            }
            throw 1;
        }

        // loop through the PerformerIDs and assign to local Comms
        int tRankCount = 0;
        for( int i=0; i<tNumGlobalRanks; i++ )
        {
            if( tPerformerIDs[i] == tLocalPerformerID )
            {
                if( i == tMyRank )
                {
                     int tNameIndex = tRankCount/tLocalPerformerCommSize;
                     mLocalPerformerName = tPerformerNames[tNameIndex];
                     mLocalCommID = tCommIndex;
                }
                tRankCount++;
                if( tRankCount % tLocalPerformerCommSize == 0 )
                {
                    tCommIndex++;
                }
            }
        }
    }

    // Did any rank not find their local comm id?
    //
    int tErrorNoComm = ( mLocalCommID == -1 ) ? 1 : 0;
    int tErrorNoCommGlobal = 0;
    MPI_Allreduce(&tErrorNoComm, &tErrorNoCommGlobal, 1, MPI_INT, MPI_MAX, mGlobalComm);
    if( tErrorNoCommGlobal ){
        if( tMyRank == 0 )
        {
                std::cout << " -- Fatal Error --------------------------------------------------------------" << std::endl;
                std::cout << "  Not all ranks were assigned to a local comm. " << std::endl;
                std::cout << " -----------------------------------------------------------------------------" << std::endl;
        }
        throw 1;
    }

    MPI_Comm_split(mGlobalComm, mLocalCommID, tMyRank, &mLocalComm);

    mPerformer = std::make_shared<Plato::Performer>(mLocalPerformerName, mLocalCommID);

    mExceptionHandler = new Plato::ExceptionHandler(mLocalPerformerName, mLocalComm, mGlobalComm);
}

/******************************************************************************/
void Interface::setPerformerOnStages()
/******************************************************************************/
{
    for(auto tStage : mStages)
    {
        if(tStage)
        {
            tStage->setPerformerOnOperations(mPerformer);
        }
    }
}

/******************************************************************************/
void Interface::initializeConsole()
/******************************************************************************/
{
    if(mConsole == nullptr)
    {
        mConsole = new Console(mLocalPerformerName, mPerformerID, Plato::Get::InputData(mInputData,"Console"), mLocalComm);
    }
}

/******************************************************************************/
void Interface::exportGraph(const Plato::SharedDataInfo & aSharedDataInfo,
                            Plato::Application* const aApplication,
                            Plato::CommunicationData & aCommunicationData) const
/******************************************************************************/
{
    if(aSharedDataInfo.isLayoutDefined("NODAL FIELD") == true)
    {
        auto tLayout = Plato::data::layout_t::SCALAR_FIELD;
        this->exportOwnedGlobalIDs(tLayout, aApplication, aCommunicationData);
    }

    if(aSharedDataInfo.isLayoutDefined("ELEMENT FIELD") == true)
    {
        auto tLayout = Plato::data::layout_t::ELEMENT_FIELD;
        this->exportOwnedGlobalIDs(tLayout, aApplication, aCommunicationData);
    }
}

/******************************************************************************/
void Interface::exportOwnedGlobalIDs(const Plato::data::layout_t & aLayout,
                                     Plato::Application* const aApplication,
                                     Plato::CommunicationData & aCommunicationData) const
/******************************************************************************/
{
    try
    {
        aApplication->exportDataMap(aLayout, aCommunicationData.mMyOwnedGlobalIDs[aLayout]);
    }
    catch(...)
    {
        mExceptionHandler->Catch();
    }
}

/******************************************************************************/
int Interface::size(const std::string & aName) const
/******************************************************************************/
{
    int tLength = 0;

    Plato::SharedData* tSharedData = mDataLayer->getSharedData(aName);
    if(tSharedData)
    {
        tLength = tSharedData->size();
    }
    else
    {
        // TODO: throw?  return zereo?
    }
    return tLength;
}

/******************************************************************************/
void Interface::Catch()
/******************************************************************************/
{
    mExceptionHandler->Catch();
}

/******************************************************************************/
void Interface::registerException(Plato::ParsingException aParsingException)
/******************************************************************************/
{
    mExceptionHandler->registerException(aParsingException);
}

/******************************************************************************/
void Interface::registerException(Plato::LogicException aLogicException)
/******************************************************************************/
{
    mExceptionHandler->registerException(aLogicException);
}

/******************************************************************************/
void Interface::registerException(Plato::TerminateSignal aTerminateSignal)
/******************************************************************************/
{
    mExceptionHandler->registerException(aTerminateSignal);
}

/******************************************************************************/
void Interface::handleExceptions()
/******************************************************************************/
{
    mExceptionHandler->handleExceptions();
}

/******************************************************************************/
void Interface::registerException()
/******************************************************************************/
{
    mExceptionHandler->registerException();
}

/******************************************************************************/
bool Interface::isDone()
/******************************************************************************/
{
    return mIsDone;
}

/******************************************************************************/
Interface::~Interface()
/******************************************************************************/
{
    if(mDataLayer)
    {
        delete mDataLayer;
        mDataLayer = nullptr;
    }
    if(mExceptionHandler)
    {
        delete mExceptionHandler;
        mExceptionHandler = nullptr;
    }
    if(mConsole)
    {
        delete mConsole;
        mConsole = nullptr;
    }
    const size_t tNumStages = mStages.size();
    for(size_t tStageIndex = 0u; tStageIndex < tNumStages; tStageIndex++)
    {
        delete mStages[tStageIndex];
    }
    mStages.clear();
}

void Interface::checkAndSetApplication(Application* aApplication)
{
    if(aApplication == nullptr)
    {
        registerException(Plato::ParsingException("Failed to create Application"));
    }
    this->handleExceptions();

    if(mPerformer)
    {
        mPerformer->setApplication(aApplication);
    }
}

} /* namespace Plato */
