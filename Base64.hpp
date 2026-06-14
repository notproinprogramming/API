#include <vector>
#include <string>


void initDecodeTable() ;

std::string encodeBase64(const std::vector<unsigned char>& data);
bool decodeBase64Line(const std::string& line,
                      std::vector<unsigned char>& output,
                      int lineNumber,
                      bool& paddingEncountered) ;

int Base64EncodeFile(const std::string& inputFile,
                     const std::string& outputFileUser);


int Base64DecodeFile(const std::string& inputFile,
                     const std::string& outputFileUser) ;

