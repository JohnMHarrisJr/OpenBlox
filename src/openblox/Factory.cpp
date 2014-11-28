#include "Factory.h"

#include "../ob_instance/Instance.h"

namespace OpenBlox{
	void Factory::addClass(const char* className, ClassMaker* const newClassMaker){
		char* key = covertToLower(className);
		std::string keystr = std::string(key);
		lokupTable[keystr] = newClassMaker;
	}

	void* Factory::create(const char* className){
		char* key = covertToLower(className);
		std::string keystr = std::string(key);

		void* result = NULL;
		std::map<std::string, ClassMaker*>::iterator it = lokupTable.find(keystr);

		if(it != lokupTable.end()){
			ClassMaker* maker = it->second;
			if(maker->isInstantiatable()){
				result = maker->getInstance();
			}
		}
		return result;
	}

	bool Factory::isA(const ob_instance::Instance* obj, const char* className){
		char* key = covertToLower(className);
		std::string keystr = std::string(key);

		std::map<std::string, ClassMaker*>::iterator it = lokupTable.find(keystr);

		if(it != lokupTable.end()){
			ClassMaker* maker = it->second;
			return maker->isA(obj);
		}
		return false;
	}

	void Factory::releaseTable(){
		for(std::map<std::string, ClassMaker*>::iterator it = lokupTable.begin(); it != lokupTable.end(); ++it){
			delete it->second;
		}
	}
}