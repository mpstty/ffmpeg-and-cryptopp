#include <cryptopp/sha3.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#include <string>
#include <iostream>

#include "utilities.h"
int get_sha3_checksum_file(const char* input, char* checksum);
int get_sha3_checksum_data(const char* input, char* checksum);

hdl_get_sha3_checksum get_sha3_checksum[]={
  get_sha3_checksum_file,
  get_sha3_checksum_data,
};

int get_sha3_checksum_file(const char* input, char* checksum){
    std::string str_checksum;
    CryptoPP::SHA3_512 hash;

    CryptoPP::FileSource((const_cast<char*>(input)), true,
                                new CryptoPP::HashFilter(
                                  hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(str_checksum), true)
                                )
                          );
    strcpy(checksum, const_cast<char*>(&str_checksum[0])); //.c_str()
    return 0;
}

int get_sha3_checksum_data(const char* input, char* checksum){
    std::string str_checksum;
    CryptoPP::SHA3_512 hash;
    CryptoPP::StringSource((const_cast<char*>(input)), true,
                                new CryptoPP::HashFilter(
                                  hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(str_checksum), true)
                                )
                          );
    strcpy(checksum, const_cast<char*>(&str_checksum[0])); //.c_str()
    return 0;
}
