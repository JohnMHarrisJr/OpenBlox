#include "ThreadScheduler.h"

namespace OpenBlox{
	Thread* ThreadScheduler::taskThread = NULL;

	static std::vector<ThreadScheduler::Task> tasks = std::vector<ThreadScheduler::Task>();
	static std::vector<ThreadScheduler::WaitingTask> waitingTasks = std::vector<ThreadScheduler::WaitingTask>();
	static std::vector<ThreadScheduler::WaitingFuncTask> waitingFuncTasks = std::vector<ThreadScheduler::WaitingFuncTask>();

	int ThreadScheduler::Delay(lua_State* L, int funcidx, long millis){
		lua_State* NL = lua_newthread(L);
		lua_pushvalue(L, funcidx);
		int r = lua_ref(L, LUA_REGISTRYINDEX);

		long curTime = currentTimeMillis();

		Task tsk = Task();
		tsk.origin = NL;
		tsk.at = curTime + millis;
		tsk.start = curTime;
		tsk.ref = r;

		enqueue_task(tsk);

		return 0;
	}

	int ThreadScheduler::Spawn(lua_State* L, int funcidx){
		return Delay(L, funcidx, 0);
	}

	int ThreadScheduler::Wait(lua_State* L, long millis){
		long curTime = currentTimeMillis();

		Task tsk = Task();
		tsk.origin = L;
		tsk.at = curTime + millis;
		tsk.start = curTime;

		enqueue_task(tsk);

		return lua_yield(L, 2);
	}

	void ThreadScheduler::AddWaitingTask(lua_State* L, int nargs){
		WaitingTask task = WaitingTask();
		task.state = L;
		task.resumeArgs = nargs;

		waitingTasks.push_back(task);
	}

	void ThreadScheduler::RunOnTaskThread(task_func* func, long millis, ...){
		va_list ap;
		va_start(ap, millis);

		WaitingFuncTask task = WaitingFuncTask();
		task.func = func;
		task.args = ap;
		task.at = currentTimeMillis() + millis;

		waitingFuncTasks.push_back(task);
		std::sort(waitingFuncTasks.begin(), waitingFuncTasks.end(), func_less_than_key());
	}

	bool ThreadScheduler::isOnTaskThread(){
		return taskThread->isCurrent();
	}

	void ThreadScheduler::enqueue_task(ThreadScheduler::Task t){
		tasks.push_back(t);
		std::sort(tasks.begin(), tasks.end(), less_than_key());
	}

	void ThreadScheduler::Tick(){
		if(!waitingTasks.empty()){
			while(!waitingTasks.empty()){
				try{
					WaitingTask task = waitingTasks.at(waitingTasks.size() - 1);
					lua_State* L = task.state;
					if(L != NULL){
						int stat = lua_resume(L, task.resumeArgs);
						if(stat != 0 && stat != LUA_YIELD){
							OpenBlox::BaseGame::getInstance()->handle_lua_errors(L);
						}
					}
					waitingTasks.pop_back();
				}catch(const std::out_of_range& e){}
			}
		}
		if(!waitingFuncTasks.empty()){
			while(!waitingFuncTasks.empty()){
				try{
					WaitingFuncTask task = waitingFuncTasks.at(waitingFuncTasks.size() - 1);
					if(task.at < currentTimeMillis()){
						task.func(task.args);
						va_end(task.args);
						waitingFuncTasks.pop_back();
					}else{
						break;
					}
				}catch(const std::out_of_range& e){}
			}
		}
		if(!tasks.empty()){
			long curTime = currentTimeMillis();
			try{
				Task task = tasks.at(tasks.size() - 1);
				if(task.at < curTime){
					lua_State* L = task.origin;
					if(L == NULL){
						return;
					}
					if(lua_status(L) != LUA_YIELD && task.ref == -1){
						tasks.pop_back();
						return;
					}

					if(task.ref != -1){
						lua_resume(L, 0);
						lua_rawgeti(L, LUA_REGISTRYINDEX, task.ref);

						long appStartTime = OpenBlox::BaseGame::appStarted();
						long elapsedTime = (curTime - task.start) / 1000;

						lua_pushnumber(L, elapsedTime);
						lua_pushnumber(L, (curTime - appStartTime) / 1000.0);

						int stat = lua_pcall(L, 2, LUA_MULTRET, 0);
						if(stat != 0 && stat != LUA_YIELD){
							OpenBlox::BaseGame::getInstance()->handle_lua_errors(L);
						}
						luaL_unref(L, LUA_REGISTRYINDEX, task.ref);
						tasks.pop_back();
						return;
					}

					long appStartTime = OpenBlox::BaseGame::appStarted();
					long elapsedTime = (curTime - task.start) / 1000;

					lua_pushnumber(L, elapsedTime);
					lua_pushnumber(L, (curTime - appStartTime) / 1000.0);

					tasks.pop_back();
					int stat = lua_resume(L, 2);
					if(stat != 0 && stat != LUA_YIELD){
						OpenBlox::BaseGame::getInstance()->handle_lua_errors(L);
					}
				}
			}catch(const std::out_of_range& e){}
		}
	}
}
