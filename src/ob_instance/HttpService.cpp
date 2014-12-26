#include "HttpService.h"

#include <curl/curl.h>

namespace ob_instance{
	struct HttpServiceClassMaker: public OpenBlox::ClassMaker{
		ob_instance::Instance* getInstance() const{
			return new HttpService;
		}

		bool isA(const ob_instance::Instance* obj){
			return (dynamic_cast<const HttpService*>(obj)) != 0;
		}

		bool isInstantiatable(){
			return false;
		}

		bool isService(bool isDataModel){
			return isDataModel;
		}
	};

	STATIC_INIT(HttpService){
		OpenBlox::BaseGame::getInstanceFactory()->addClass(ClassName, new HttpServiceClassMaker());

		registerLuaClass(LuaClassName, register_lua_metamethods, register_lua_methods, register_lua_property_getters, register_lua_property_setters, register_lua_events);
	}

	char* HttpService::ClassName = "HttpService";
	char* HttpService::LuaClassName = "luaL_Instance_HttpService";

	HttpService::HttpService() : Instance(){
		Name = ClassName;
	}

	HttpService::~HttpService(){}

	size_t HttpService::write_data(void* ptr, size_t size, size_t nmemb, struct response_body* data){
		size_t index = data->size;
		size_t n = (size * nmemb);
		char* tmp;

		data->size += n;

		tmp = (char*)realloc(data->data, data->size + 1);

		if(tmp){
			data->data = tmp;
		}else{
			if(data->data){
				delete[] data->data;
			}
			LOGE("[HttpService] Failed to allocate memory.");
			return 0;
		}

		memcpy((data->data + index), ptr, n);
		data->data[data->size] = '\0';

		return n;
	}

	char* HttpService::GetAsync(const char* url, bool nocache){
		struct response_body body;
		body.size = 0;
		body.data = new char[4096];
		if(body.data == NULL){
			LOGE("[HttpService] Failed to allocate memory.");
			return NULL;
		}

		body.data[0] = '\0';

		CURL* curl;
		CURLcode res;

		curl = curl_easy_init();
		if(curl){
			curl_easy_setopt(curl, CURLOPT_URL, url);

			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);

			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

			res = curl_easy_perform(curl);
			if(res != CURLE_OK){
				LOGE("[HttpService] cURL Error: %s", curl_easy_strerror(res));
			}

			curl_easy_cleanup(curl);

			return body.data;
		}
		return NULL;
	}

	char* HttpService::UrlEncode(const char* input){
		CURL* curl;

		curl = curl_easy_init();
		if(curl){
			char* encoded = curl_easy_escape(curl, input, 0);

			size_t thingLen = strlen(encoded) + 1;
			char* newGuy = new char[thingLen];
			strcpy(newGuy, encoded);
			newGuy[thingLen] = '\0';

			curl_free(encoded);
			curl_easy_cleanup(curl);

			return newGuy;
		}
		return NULL;
	}

	int HttpService::wrap_lua(lua_State* L){
		HttpService** udata = (HttpService**)lua_newuserdata(L, sizeof(*this));
		*udata = this;

		luaL_getmetatable(L, LuaClassName);
		lua_setmetatable(L, -2);

		return 1;
	}

	Instance* HttpService::cloneImpl(){
		return NULL;
	}

	char* HttpService::getClassName(){
		return ClassName;
	}

	void HttpService::register_lua_methods(lua_State* L){
		luaL_Reg methods[]{
			{"GetAsync", [](lua_State* L)->int{
				Instance* inst = checkInstance(L, 1);
				if(HttpService* hs = dynamic_cast<HttpService*>(inst)){
					const char* url = luaL_checkstring(L, 2);
					bool nocache = false;
					if(!lua_isnoneornil(L, 2)){
						if(lua_isboolean(L, 2)){
							nocache = lua_toboolean(L, 3);
						}else{
							nocache = true;
						}
					}

					char* body = hs->GetAsync(url, nocache);

					if(body){
						lua_pushstring(L, body);
					}else{
						lua_pushnil(L);
					}
					return 1;
				}
				return luaL_error(L, COLONERR, "GetAsync");
			}},
			{"UrlEncode", [](lua_State* L)->int{
				Instance* inst = checkInstance(L, 1);
				if(HttpService* hs = dynamic_cast<HttpService*>(inst)){
					const char* inputStr = luaL_checkstring(L, 2);

					char* output = hs->UrlEncode(inputStr);

					if(output){
						lua_pushstring(L, output);
					}else{
						lua_pushnil(L);
					}
					return 1;
				}
				return luaL_error(L, COLONERR, "UrlEncode");
			}},
			{NULL, NULL}
		};
		luaL_register(L, NULL, methods);

		Instance::register_lua_methods(L);
	}
}