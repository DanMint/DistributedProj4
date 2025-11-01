#ifndef CLIENT_H
#define CLIENT_H

#include <string>

class DHTClient {
private:
    int sock;                      // Socket for client connection
    std::string bootstrapServer;   // Name/address of bootstrap server
    int testCase;                  // Test case number to run
    
    // Get the IP address of the bootstrap server
    std::string getBootstrapIP();
    
    // Connect to the bootstrap server
    bool connectToBootstrap(const std::string& bootstrapIP);
    
    // Send a request and receive response
    std::string sendRequest(const std::string& request);
    
public:
    // Constructor
    DHTClient(const std::string& bootstrap, int testCase);
    
    // Destructor
    ~DHTClient();
    
    // Run the specified test case
    void runTestCase();
};

#endif // CLIENT_H