/*******************************************************************
	SmartState State Machine Code. Do not Distribute. DO NOT EDIT.
	(c) ApeSoft [www.SmartStateStudio.com]
*******************************************************************/

#ifndef SS_API_H
#define SS_API_H

/*STL Includes*/
#include <list>
#include <map>
#include <string>

using std::list;
using std::map;
using std::string;
using std::ostream;
using std::istream;

namespace smartstate
{
	struct Mesg
	{
		Mesg();

		Mesg(unsigned int m, int w, int l);

		unsigned int message;
		int wParam;
		int lParam;
	};

	class BaseState;

	typedef list<BaseState*> BaseStateList;
	typedef map<string, BaseState*> BaseStateMap;

	struct PostedMesg
	{
		string stateName;
		Mesg mesg;
	};

	typedef list<PostedMesg> PostedMesgList;
	enum EStateType {eSuper, eSub, eConc};


	/*****************************************************************************/
	/* STATEMGR BEGIN*/
	/*****************************************************************************/
	/* Class: StateMgr
	 *Description: This is a base class for handling the state transitions.
	 *The generated code will specialize this class.
	 *Context should create an instance of the generated class.
	 *This class defines the interface to be used by the Context class.
	 */
	class StateMgr
	{
		friend class BaseState;

		protected:

			/*Constructor
			*
			*Param: name - Name of the object.
			*/
			StateMgr(const string& name);

		public:

			/*Destructor
			*/
			virtual ~StateMgr();

		private:

			/*No copy & assignment allowed.
			*/
			StateMgr(const StateMgr& );
			StateMgr& operator=(const StateMgr& );

		/*METHODS - For Context
		*/
		public:

			/*
			*Method: start
			*Description: Runs the StateMgr and initialize all initial states.
			*Param: None
			*Return: None.
			*See: 'serialise' function for initialising to the last saved state.
			*/
			void start();

			/*
			*Method: getName
			*Description: Returns the name of the StateMgr object.
			*Param: None
			*Return: Name of StateMgr object.
			*/
			const string& getName();

			/*
			*Method: postEvent
			*Description: Interface used by Context to post events.
			*Param: message - the event for state transition.
			*Param: wParam - additional information with the event.
			*				  [default = 0]
			*Param: lparam - additional information with the event.
			*				  [default = 0]
			*Return: None
			*Exception: std::string
			*			 Possible reason:
			*			 1) StateMgr is in FinalState or not running.
			*			 2) Internal messaging (PostMessage) specifies an
			*			 invalid target state name.
			*			 3) Internal logic error (may be an invalid model)
			*/
			virtual void postEvent(unsigned int message, int wParam = 0,
				int lParam = 0) throw (std::string);

			/*
			*Method: reInit
			*Description: Interface used by Context to reinitialize the
			*			   state of StateMgr. It resets all active states
			*			   and calls onEntry() of each state. Usually used
			*			   to reInitialize when the state goes to FinalState.
			*Param: none
			*Return: None
			*Exception: None
			*/
			void reInit();

			/*
			*Method: isRunning
			*Description: Interface used by Context to check the status of
			*			   StateMgr. Returns true when its active and false
			*			   when its in FinalState.
			*Param: none
			*Return: Either
			*		  true  - Running, can send messges
			*		  false - In FinalState, need to be reinitialized.
			*Exception: None
			*/
			virtual bool isRunning() const;

			/*
			*Method: serialise
			*Description: Persist the state machine to the ostream object.
			*			We can initialise the state machine to the same state later.
			*Param: outStream - The output stream to serialise the machine
			*		Can be a physical file opened (fstream) or an inmemory
			*		string stream (ostringstream) or any other ostream.
			*/
			void serialise(ostream& outStream) const;

			/*
			*Method: serialise
			*Description: Load the state machine from the istream object.
			*			The machine will be running after this operation.
			*			Calling start() or reInit() after serialise will
			* 			reset the machine to initial states.
			*
			*Param: inStream - The input stream to serialise the machine
			*		Can be a physical file opened (fstream) or an inmemory
			*		string stream (istringstream) or any other istream.
			*		Can serialise only a previously saved state machine.
			*
			*Return: None
			*/
			void serialise(istream& inStream);

			/*
			*Method: setDebugLog
			*Description: To specify user defined output stream for logging
			*debug messages. Effective ONLY if the code generation is done with
			*-g flag while compiling. Else reduntant.
			*Optional API only if needed to override the default (standard console)
			*
			*Param: logStream - Out stream for debug data logging.
			*	Default to standard console (cout).
			*
			*Return: None
			*/
			void setDebugLog(ostream* logStream);

		/*METHODS - For Internal Classes
		*/
		public:

			/*
			*Method: executeEntry
			*Description: Interface used by generated classes for entering a state
			*			   Should not be called from Context.
			*Param: root - The common root state reference.
			*Param: nextState - The name of next state.
			*Return: None.
			*/
			void executeEntry(const BaseState* root, const string& nextState);

			/*
			*Method: executeExit
			*Description: Interface used by generated classes for exiting a state.
			*			   Should not be called from Context.
			*Param: currState - The name of current state.
			*Param: nextState - The name of next state.
			*Return: The common root state reference.
			*/
			const BaseState* executeExit(const string& currState, const string& nextState);

			/*Method: debugLog
			*Description: Log the generated debug message using the out stream.
			*See: setDebugLog for more information.
			*Param: str - the message to log
			*/
			void debugLog(const string& str);

		private:

			/*
			*Method: getRoot
			*Description: Returns the common root object reference.
			*Param: leaf1 - the first leaf
			*Param: leaf2 - the second leaf
			*Return: The common root object reference.
			*/
			const BaseState* getRoot(const BaseState* leaf1, const BaseState* leaf2);

			/*
			*Method: constructTree
			*Description: Constructs the tree of object references.
			*Param: root - The common root object reference
			*Param: state - the leaf
			*Param: tree - the tree as out param
			*Return: None
			*/
			void constructTree(const BaseState* root, const BaseState* state,
				BaseStateList& tree);

			/*
			*Method: constructInitialTree
			*Description: Constructs the initial state tree.
			*Param: tree - the tree as out param
			*Return: None
			*/
			void constructInitialTree(BaseStateList& tree);

			/*
			*Method: registerState
			*Description: Registers the state name with its object reference.
			*Param: stateName - name of the state.
			*Param: stateRef - object reference.
			*Return: None
			*/
			void registerState(const string& stateName, BaseState* stateRef);

			/*
			*Method: removeActiveStates
			*Description: Removes the active states in between the given state
			*			   and the root and its child states.
			*Param: state - the leaf state reference.
			*Param: root - the root object reference.
			*Return: None
			*/
			void removeActiveStates(const BaseState* state, const BaseState* root);

			/*
			*Method: fireMessage
			*Description: Sends a message to the target if specified else to all
			*Param: mesg - the message to send.
			*Param: target - the target Conurrent state object ref.
			*Return: None
			*/
			void fireMessage(const Mesg& mesg, BaseState* target=0);

			/*
			*Method: queueMessage
			*Description: Queue the posted internal messages for delivery.
			*Param: postMsg - the posted message.
			*Return: None
			*/
			void queueMessage(PostedMesg postMsg);

			/*
			*Method sendPostedMessages
			*Description: Process all internally posted messages
			*Return: None
			*/
			void sendPostedMessages();


		/*ATRIBUTES
		*/
		protected:

			/*List of concurrent states.
			*/
			BaseStateList myConcStateList;

		private:

			/*Map of name Vs object of registerd states.
			*/
			BaseStateMap myStateMap;

			/*Currently active states
			*/
			BaseStateList myActiveStatesList;

			/*Current status
			*/
			bool myStatus;

			/*Posted messages list.
			*/
			PostedMesgList myPostedMesgList;

			/*name of the StateMgr object.
			*/
			string myName;

			/*Processing a message or not.
			*/
			bool myBusyStatus;

			/*out stream for debug logging if -g is specified while code generation
			*/
			ostream* myDebugLogStream;
	};

	/*INLINES
	*/
	inline const string& StateMgr::getName()
	{
		return myName;
	}

	/*****************************************************************************/
	/* STATEMGR END*/
	/*****************************************************************************/

	/*****************************************************************************/
	/* BASESTATE BEGIN*/
	/*****************************************************************************/
	class StateMgr;

	class BaseState
	{
		friend class StateMgr;

		protected:
			BaseState();
			BaseState(const string& name, BaseState* parent, StateMgr* mgr);
		public:
			virtual ~BaseState();

		private:
			BaseState(const BaseState& );
			BaseState& operator=(const BaseState& );

		public:
			BaseState* getParent() const;
			const string& getName() const;
			EStateType getType() const;

			bool isParent(const BaseState* state) const;
			void getInitialStates(BaseStateList& iniList) const;

		protected:
			virtual void onMessage(const Mesg& mesg);
			void setType(EStateType type);
			void postMessage(unsigned int message, int wParam = 0, int lParam = 0);
			void postMessage(string targetState, unsigned int message, int wParam = 0, int lParam = 0);

			virtual void onEntry();
			virtual void onExit();

			static bool isInList(const BaseState* state, const BaseStateList& aList);

		protected:
			const string myName;
			BaseState* myParent;
			BaseStateList mySubStates;
			EStateType myType;
			StateMgr* myMgr;

			bool myHistory;
			BaseState* myHistoryState;
	};

	#define POST postMessage

	inline BaseState* BaseState::getParent() const
	{
		return myParent;
	}

	inline const string& BaseState::getName() const
	{
		return myName;
	}

	inline EStateType BaseState::getType() const
	{
		return myType;
	}

	/*****************************************************************************/
	/* BASESTATE END*/
	/*****************************************************************************/

} /*end namespace smartstate*/

#endif
