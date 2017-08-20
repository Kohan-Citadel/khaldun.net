#ifndef _OS_KVREADER_H
#define _OS_KVREADER_H

#include <OS/OpenSpy.h>
#include <unordered_map>
#include <iterator>
/*
	This is not thread safe!!

	Because of:
		GetHead
*/
namespace OS {
	class KVReader {
	public:
		KVReader(std::string kv_pair, char delim = '\\');
		~KVReader();
		std::string GetKeyByIdx(int n);
		std::string GetValueByIdx(int n);
		int 		GetValueIntByIdx(int n);
		std::pair<std::string, std::string> GetPairByIdx(int n);
		std::string 						GetValue(std::string key);
		int 								GetValueInt(std::string key);
		std::pair<std::unordered_map<std::string, std::string>::const_iterator, std::unordered_map<std::string, std::string>::const_iterator> GetHead() const;
		bool HasKey(std::string name);
	private:
		int GetIndex(int n); //map internal index to external index
		std::unordered_map<std::string, std::string> m_kv_map;
	};
}
#endif //_OS_KVREADER_H