#include <iostream>
#include <XmlRpcCpp.h>

#define NAME "ConceptNet Server Client"
#define VERSION "1.0"
#define SERVER_URL "http://localhost:8000/"

int main(int argc, char **argv) {

  if(argc == 1) {
    std::cout << "Query term is not specified" << std::endl;
    exit(1);
  }
  XmlRpcValue result;
  XmlRpcClient::Initialize(NAME, VERSION);
  XmlRpcValue paramArray = XmlRpcValue::makeArray();
  paramArray.arrayAppendItem(XmlRpcValue::makeString(argv[1]));
  try {
    XmlRpcClient conceptServer(SERVER_URL);
    result = conceptServer.call("get_context", paramArray);
  } catch (XmlRpcFault& fault) {
    std::cerr << "XML-RPC error: (" << fault.getFaultCode() << ") " << fault.getFaultString() << std::endl;
    exit(1);
  }

  XmlRpcValue::int32 contextSize = result.structGetValue("size").getInt();
  std::cout << "Context size: " << contextSize << std::endl;
  XmlRpcValue context = result.structGetValue("context").getArray();
  for(int i = 0; i < contextSize; i++) {
    XmlRpcValue relConcept = context.arrayGetItem(i);
    std::string relation = relConcept.structGetValue("relation").getString();
    std::string concept = relConcept.structGetValue("concept").getString();
    std::cout << relation << ": " << concept << std::endl;
  }
  XmlRpcClient::Terminate();
  return 0;
}
