#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"
#include "DataBase.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens") {
  
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response) {

    // get the request body and respond with token.
    string uname, upassword;
    User *u;

    WwwFormEncodedDict dict = request->formEncodedBody();
    uname = dict.get("username");
    upassword = dict.get("password");

    cout << "username: " << uname << endl;
    cout << "password: " << upassword << endl;

    // user name are all lower case letters
    for (size_t  i = 0; i < uname.length(); i++) {
        if (!islower(uname[i])) {
	    response->setStatus(400);
	    return;
	}
    }
	 

    // check users DATABASE - with user id and auth tokens.
    
    std::map <std::string, User *> ::iterator it;
    it = m_db->users.find(uname);

    if (it != m_db->users.end() && (it->second->password != upassword)) {
        // login with incorrect password, return errors.
	response->setStatus(403);
        return;
    }


    // use string utils to create randomized user id and auth token.
    // if new user login, create userid for the first time.
    string userid;
    if (it == m_db->users.end()) {
        u = new User;
        u->username = uname;
        u->password = upassword;
        userid = StringUtils::createUserId();
        u->user_id = userid;

        // save the user credential and info into the DATABASE
        m_db->users.insert({uname, u});
    }
    else {
	// use known user id generated previously.
        userid = it->second->user_id;
    }

    // generate auth token for each individual session
    string authtoken = StringUtils::createAuthToken();

    Document document;
    
    Document::AllocatorType& a = document.GetAllocator();

    Value o;
    o.SetObject();
    o.AddMember("auth_token", authtoken, a);
    o.AddMember("user_id", userid, a);

    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the correct response objects.
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));

    return;
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {

}
