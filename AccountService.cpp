#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}


void createResponse(HTTPResponse *response, User *u) {

    // now generate response
    Document document;
    Document::AllocatorType& a = document.GetAllocator();

    Value o;
    o.SetObject();
    o.AddMember("balance", u->balance, a);
    o.AddMember("email", u->email, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the correct response objects.
    response->setStatus(200);
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));

    return;
}

// This can be a AccountService method.
User * getUserAndCheckForErrors(HTTPRequest *request, User *u) {

    string user_id;
    string paths = request->getPath();

    
    std::string pathprefix = "/users";
    std::size_t pos = paths.find(pathprefix);

    // pos has to equial the begin of the path, or error.
    pos += pathprefix.length();

    // find token to be deleted from path/url command, ignore "/"
    user_id = paths.substr(pos+1);

    if (u->user_id != user_id) {
	// get user info for a different user id
        throw ClientError::forbidden();
    }

    return u;
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {

    User *u;

    u = getAuthenticatedUser(request);
    getUserAndCheckForErrors(request, u);
    
    // now generate response
    createResponse(response, u);

    return;
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {

    string email;
    User *u;

    u = getAuthenticatedUser(request);
    getUserAndCheckForErrors(request, u);

    // need update user's email address
    WwwFormEncodedDict dict = request->formEncodedBody();
    email = dict.get("email");
    u->email = email;

    // now generate response
    createResponse(response, u);
   
}
