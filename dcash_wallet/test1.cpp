#define RAPIDJSON_HAS_STDSTRING 1
#include <assert.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

#include "HttpClient.h"

using namespace std;

int PORT = 8080;

std::string PUBLISHABLE_KEY("pk_test_51Ivo89Hx2wJq3xlEU3wc6xLSRFkNOlbKiHF4sV55IheAlX6W3fGtbe1TfxGym8Wozmx1ld3ddf7IiX1bb4qOffhC00n5xDlKOc");
//
// XXX - this shall work - use curl command worked.
//
void get_token_from_stripe(void)
{
    HTTPClientResponse *response;
    Document *d;

    // need SSL.
    HttpClient client("api.stripe.com", 443, true);
    client.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);
    client.set_header("Content-Type", "application/x-www-form-urlencoded");

    string token_body;

    WwwFormEncodedDict dict;
    dict.set("card[number]", "4242424242424242");
    dict.set("card[exp_month]", "2");
    dict.set("card[exp_year]", "2024");
    dict.set("card[cvc]", "314" );
    token_body = dict.encode();

    // send deposit request for token with stripe.
    response = client.post("/v1/tokens", token_body);

    // check response for tokenization
    if (!response->success()) {
        cout << "Error of response" << endl;
    }

    d = response->jsonBody();
    string stripe_token = (*d)["id"].GetString();
    delete response;
    delete d;

}


int main(int argc, char *argv[]) {
  cout << "Making http request" << endl;
  
  // HttpClient client("localhost", PORT);
  // HTTPResponse *response = client.get("/hello_world.html");
  // assert(response->status() == 200);
  // delete response;
  
  // cout << "passed" << endl;
  
  get_token_from_stripe();

  return 0;
}
