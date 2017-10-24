/*********************************************************************
	SmartState State Machine Code. Do not Distribute. DO NOT EDIT.
	(c) ApeSoft [www.SmartStateStudio.com]
**********************************************************************/

//#pragma warning(disable: 4786)
//#pragma warning(disable: 4290)

#include <iostream>

#include "ss_api.hxx"

#define SS_SERIALISE_BEG_TAG "SS_BEG"
#define SS_SERIALISE_END_TAG "SS_END"

/***************************************************************************/
namespace smartstate
{

using namespace std;

/***************************************************************************/
Mesg::Mesg()
{
	message = 0;
	wParam  = 0;
	lParam  = 0;
}

Mesg::Mesg(unsigned int m, int w, int l)
{
	message = m;
	wParam  = w;
	lParam  = l;
}


/***************************************************************************/
StateMgr::StateMgr(const string& name)
: myStatus(false),
  myName(name),
  myBusyStatus(false)
{
	myDebugLogStream = &cout;
}

StateMgr::~StateMgr()
{
	//destroy all conc machines.
	BaseStateList::iterator it = myConcStateList.begin();
	BaseStateList::iterator end = myConcStateList.end();

	for(; it != end; it++)
	{
		delete (*it);
	}
}

/***************************************************************************/
void StateMgr::start()
{
	myActiveStatesList.clear();

	//fill all initial active states.
	BaseStateList::iterator it = myConcStateList.begin();
	BaseStateList::iterator end = myConcStateList.end();
	for(; it != end; it++)
	{
		(*it)->getInitialStates(myActiveStatesList);
	}

	//call onEntry on initial states and its links
	BaseStateList tree;
	constructInitialTree(tree);

	it = tree.begin();
	end = tree.end();
	for(; it != end; it++)
	{
		(*it)->onEntry();
	}

	//if any posted internal message (from onEntry of initial state) fire now
	sendPostedMessages();

	myStatus = true; //running
}

/***************************************************************************/
void StateMgr::postEvent(unsigned int message, int wParam, int lParam)
throw (std::string)
{
	if(myStatus == false)
	{
		throw std::string("Not Running");
	}

	if(myBusyStatus)
	{
		PostedMesg postmesg;
		postmesg.stateName = "*"; //to all
		postmesg.mesg.message = message;
		postmesg.mesg.wParam = wParam;
		postmesg.mesg.lParam = lParam;

		myPostedMesgList.push_back(postmesg); 
		return;
	}
	
	myBusyStatus = true;

	myPostedMesgList.clear(); //empty the q

	Mesg aMesg(message, wParam, lParam);

	fireMessage(aMesg);

	//if any posted internal message fire now
	sendPostedMessages();

	myBusyStatus = false;
}

/***************************************************************************/
void StateMgr::sendPostedMessages()
{
	PostedMesgList::iterator pIt = myPostedMesgList.begin();
	for(; pIt != myPostedMesgList.end(); pIt++)
	{
		if((*pIt).stateName == "*")
		{
			fireMessage((*pIt).mesg, 0); //to all
			continue;
		}

		BaseStateMap::iterator itM = myStateMap.find((*pIt).stateName);
		if(itM == myStateMap.end())
		{	//not found.
			myBusyStatus = false;
			throw std::string("Unable to deliver message to " + (*pIt).stateName + " , state not found.");
		}

		BaseState* pState = (*itM).second;

		fireMessage((*pIt).mesg, pState); 
	}
}

/***************************************************************************/
void StateMgr::reInit()
{
	start();
}

/***************************************************************************/
bool StateMgr::isRunning() const
{
	return myStatus;
}

/***************************************************************************/
void StateMgr::serialise(ostream& outStream) const
{
	//Save all active states
	outStream << SS_SERIALISE_BEG_TAG << " " 
		<< myActiveStatesList.size() << " ";

	BaseStateList::const_iterator it = myActiveStatesList.begin();
	BaseStateList::const_iterator end = myActiveStatesList.end();
	for(; it != end; it++)
	{
		const BaseState* pState = (*it);

		outStream << pState->getName().c_str() << " ";
	}

	outStream << SS_SERIALISE_END_TAG << " ";
}

/***************************************************************************/
void StateMgr::serialise(istream& inStream)
{
	//Load all active states
	string item;

	inStream >> item;
	if(item != SS_SERIALISE_BEG_TAG)
		throw std::string("Not a valid input stream. Cannot serialise");

	myStatus = false; //not running
	myActiveStatesList.clear();

	int nbActiveStates = 0;
	inStream >> nbActiveStates;
	if(nbActiveStates <= 0)
	{
		throw std::string("No active states found in input stream. Cannot serialise");
	}

	for(int i=0; i<nbActiveStates; i++)
	{
		inStream >> item;

		//find the state
		BaseStateMap::iterator it = myStateMap.find(item);
		if(it == myStateMap.end())
		{
			myActiveStatesList.clear();
			throw std::string("Invalid state found in input stream : " + item);
		}

		BaseState* pState = (*it).second;

		myActiveStatesList.push_back(pState);
	}

	inStream >> item;
	if(item != SS_SERIALISE_END_TAG)
	{
		myActiveStatesList.clear();
		throw std::string("Not a valid input stream. Cannot serialise");
	}

	myStatus = true; //running
}

/***************************************************************************/
void StateMgr::setDebugLog(ostream* logStream)
{
	myDebugLogStream = logStream;
}

/***************************************************************************/
void StateMgr::fireMessage(const Mesg& mesg, BaseState* target)
{
	try
	{
		BaseStateList tempList = myActiveStatesList;
		BaseState* pState;

		BaseStateList::iterator it = tempList.begin();
		BaseStateList::iterator end = tempList.end();
		for(; it != end; it++)
		{
			pState = (*it);

			//onMessage may change the contents of activestatelist.
			//so send only if the state is still in active list
			if(BaseState::isInList(pState, myActiveStatesList))
			{
				if(target == 0)
				{
					pState->onMessage(mesg);
				}
				else
				{
					if(pState->isParent(target))
					{
						pState->onMessage(mesg);
					}
				}
			}
		}
	}
	catch(std::string&)
	{
		throw;
	}
	catch(...)
	{
		throw std::string("Internal Error, Unexpected");
	}
}

/***************************************************************************/
void StateMgr::registerState(const string& stateName, BaseState* stateRef)
{
	myStateMap[stateName] = stateRef;
}

/***************************************************************************/
const BaseState* StateMgr::executeExit(const string& currState, const string& nextState)
{
	BaseStateMap::iterator it = myStateMap.find(currState);
	if(it == myStateMap.end())
	{
		throw std::string("Invalid current state : " + currState);
	}

	BaseState* caller = (*it).second;

	//FinalState
	if(nextState == "FinalState")
	{
		//finished
		myActiveStatesList.clear();
		caller->onExit();
		return 0; //so that executeEntry wont call any thing
	}

	it = myStateMap.find(nextState);
	if(it == myStateMap.end())
	{
		throw std::string("Invalid next state : " + nextState);
	}

	BaseState* nextStateRef = (*it).second;

	const BaseState* root = getRoot(caller, nextStateRef);

	//remove this state (and its children if any) from active list
	removeActiveStates(caller, root);

	BaseState* pState = caller;
	while(pState != root)
	{
		pState->onExit();
		pState = pState->getParent();

		if(pState == 0)
		{
			throw std::string("Internal Logic Error");
		}
	}
	
	return root;
}

/***************************************************************************/
void StateMgr::executeEntry(const BaseState* root, const string& nextState)
{
	if(root == 0 || nextState == "FinalState")
	{
		myStatus = false; //not running
		return;
	}

	BaseStateMap::iterator it = myStateMap.find(nextState);
	if(it == myStateMap.end())
	{
		throw std::string("Invalid next state : " + nextState);
	}

	BaseState* nextStateRef = (*it).second;

	//Fill from root to nextStateRef including the links in between
	//if the nextState is not a leaf.. get all its initial states including
	//the links in between in the tree. Duplication handled by this method.
	BaseStateList tree;
	constructTree(root, nextStateRef, tree);

	//Add the new State (if leaf or its kids) to active list
	nextStateRef->getInitialStates(myActiveStatesList);

	//Call onEntry on each one in the list.
	BaseStateList::iterator lit = tree.begin();
	BaseStateList::iterator end = tree.end();
	for(; lit != end; lit++)
	{
		(*lit)->onEntry();
	}
}

/***************************************************************************/
const BaseState* StateMgr::getRoot(const BaseState* leaf1, const BaseState* leaf2)
{
	const BaseState* pState = leaf1;
	const BaseState* aState;
	
	while(pState) //when it reaches top it will be 0
	{
		aState = leaf2;
		while(aState)//when it reaches top it will be 0
		{
			if(pState == aState) 
			{	//common root
				return pState;
			}

			aState = aState->getParent();
		}

		pState = pState->getParent();
	}

	//no common root
	std::string err = "Invalid transition from ";
	err += leaf1->getName(); 
	err += " to ";
	err += leaf1->getName();
	err += ", no common super state.";
	throw err;

	return 0; 
}

/***************************************************************************/
void StateMgr::constructTree(const BaseState* root, const BaseState* state, BaseStateList& tree)
{
	//if the state passed is not a leaf.. get all its initial states including
	//the links in between in the tree. Duplication handled here.
	const BaseState* pState = state;
	while(pState != root)
	{
		tree.push_front(const_cast<BaseState*>(pState)); //store at top
		pState = pState->getParent();

		if(pState == 0)
		{
			throw std::string("Internal Logic Error");
		}
	}

	BaseStateList iniStatesList;
	state->getInitialStates(iniStatesList);
	
	BaseStateList::iterator it = iniStatesList.begin();
	BaseStateList::iterator end = iniStatesList.end();

	for(; it != end; it++)
	{
		pState = (*it); //the initial state
		BaseStateList aTree;

		while(pState != state) //till state
		{
			if(!BaseState::isInList(pState, tree)) //avoid duplication
			{
				aTree.push_front(const_cast<BaseState*>(pState)); //store at top
			}
			
			pState = pState->getParent();
		}

		//add to original tree
		tree.insert(tree.end(), aTree.begin(), aTree.end());
	}
}

/***************************************************************************/
void StateMgr::constructInitialTree(BaseStateList& tree)
{	//should be called only from start

	BaseStateList::iterator it = myActiveStatesList.begin();
	BaseStateList::iterator end = myActiveStatesList.end();

	BaseState* pState;
	for(; it != end; it++)
	{
		pState = (*it); //the initial state
		BaseStateList aTree;

		while(pState != 0) //till top
		{
			if(!BaseState::isInList(pState, tree)) //avoid duplication
			{
				aTree.push_front(const_cast<BaseState*>(pState)); //store at top
			}
			
			pState = pState->getParent();
		}

		tree.insert(tree.end(), aTree.begin(), aTree.end());
	}
}

/***************************************************************************/
void StateMgr::removeActiveStates(const BaseState* state, const BaseState* root)
{
	//remove caller first
	BaseStateList::iterator it = myActiveStatesList.begin();
	BaseStateList::iterator end = myActiveStatesList.end();
	for(; it != end; it++)
	{
		if((*it) == state)
		{
			myActiveStatesList.erase(it);
			break;
		}
	}

	//remove all its sub states which are active now..
	//remove its parents active state also till root.
	const BaseState* pState = state;
	while(pState != root)
	{
		it = myActiveStatesList.begin();
		while(it != myActiveStatesList.end())
		{
			if((*it)->isParent(pState))
			{
				//Store history information (only if parent is a super state and history is set)
				BaseState* pParentState = (*it)->getParent();
				if((pParentState->getType() == eSuper) && (pParentState->myHistory == true))
				{
					pParentState->myHistoryState = (*it);
				}

				it = myActiveStatesList.erase(it);
			}
			else
			{
				it++;
			}
		}

		pState = pState->getParent();

		if(pState == 0)
		{
			throw std::string("Internal Logic Error 11");
		}
	}
}

/***************************************************************************/
void StateMgr::queueMessage(PostedMesg postMsg)
{
	myPostedMesgList.push_back(postMsg);
}

/***************************************************************************/
void StateMgr::debugLog(const string& str)
{
	if (myDebugLogStream)	// added by Craig Scratchley, Simon Fraser University, Canada
							// set myDebugLogStream to NULL if you want to turn off the log
		(*myDebugLogStream) << "[SMARTSTATE_DEBUG] " << str << endl;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
BaseState::BaseState(const string& name, BaseState* parent, StateMgr* mgr)
: myName(name),
  myParent(parent),
  myType(eSub),
  myMgr(mgr),
  myHistory(false),
  myHistoryState(0)
{
	myMgr->registerState(myName, this);
}

BaseState::BaseState()
{
}

BaseState::~BaseState()
{
	//destroy all sub states.
	BaseStateList::iterator it = mySubStates.begin();
	BaseStateList::iterator end = mySubStates.end();

	for(; it != end; it++)
	{
		delete (*it);
	}
}

/***************************************************************************/
bool BaseState::isParent(const BaseState* state) const
{
	if(myParent == state)
	{
		return true;
	}

	if(myParent == 0)
	{	//top most
		return false;
	}

	return myParent->isParent(state);
}

/***************************************************************************/
void BaseState::getInitialStates(BaseStateList& iniList) const
{
	if(myType == eSuper)
	{
		//return history state if valid
		if(myHistoryState)
		{
			myHistoryState->getInitialStates(iniList);
		}
		else
		{
			(*mySubStates.begin())->getInitialStates(iniList);
		}
	}
	else if(myType == eConc)
	{
		BaseStateList::const_iterator it = mySubStates.begin();
		BaseStateList::const_iterator end = mySubStates.end();
		for(; it != end; it++)
		{
			(*it)->getInitialStates(iniList);
		}
	}
	else //sub
	{
		iniList.push_back((BaseState*)this);
	}
}

/***************************************************************************/
void BaseState::onMessage(const Mesg& mesg)
{
	//does nothing
}

/***************************************************************************/
void BaseState::setType(EStateType type)
{
	myType = type;
}

/***************************************************************************/
void BaseState::onEntry()
{
	//does nothing
}

/***************************************************************************/
void BaseState::onExit()
{
	//does nothing
}

/***************************************************************************/
void BaseState::postMessage(unsigned int message, int wParam, int lParam)
{
	//find conc state name.
	BaseState* pState = this;
	BaseState* parent = this;
	while(parent) //till root
	{
		pState = parent;
		parent = pState->getParent();
	}

	const string& concName = pState->getName();

	PostedMesg postmesg;
	postmesg.stateName = concName;
	postmesg.mesg.message = message;
	postmesg.mesg.wParam = wParam;
	postmesg.mesg.lParam = lParam;

	myMgr->queueMessage(postmesg);
}

/***************************************************************************/
void BaseState::postMessage(string targetState, unsigned int message, int wParam, int lParam)
{
	PostedMesg postmesg;
	postmesg.stateName = targetState;

	postmesg.mesg.message = message;
	postmesg.mesg.wParam = wParam;
	postmesg.mesg.lParam = lParam;

	myMgr->queueMessage(postmesg);
}

/***************************************************************************/
bool BaseState::isInList(const BaseState* state, const BaseStateList& aList)
{
	BaseStateList::const_iterator it = aList.begin();
	BaseStateList::const_iterator end = aList.end();

	for(; it != end; it++)
	{
		if((*it) == state)
		{
			return true;
		}
	}

	return false;
}


}
