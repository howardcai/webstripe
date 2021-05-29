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

    // cout << "username: " << uname << endl;
    // cout << "password: " << upassword << endl;

    // user name are all lower case letters
    for (size_t  i = 0; i < uname.length(); i++) {
        if (!islower(uname[i])) {
            throw ClientError::badRequest();
	    // response->setStatus(400);
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


    // if new user login, create userid for the first time.
    string userid;
    if (it == m_db->users.end()) {
        u = new User;
        u->username = uname;
        u->password = upassword;
	
        // use string utils to create randomized user id and auth token.
        userid = StringUtils::createUserId();
        u->user_id = userid;

        // save the user credential and info into the DATABASE
        m_db->users.insert({uname, u});
    }
    else {
	// use known user id generated previously.
        userid = it->second->user_id;
	u = it->second;
    }

    // generate auth token for each individual session.
    // this imply you can have same user login simultaneously with multiple sessions.
    string authtoken = StringUtils::createAuthToken();

    // need to remeber each auth token for each session
    // each auth token refer to the same user profile information.
    m_db->auth_tokens.insert({authtoken, u});   

    // now generate response
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
    response->setStatus(201);
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));

    return;
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response) {
    string authtoken, deltoken;
    std::map<std::string, User *> ::iterator it;
    User *u;

    u = getAuthenticatedUser(request);

    // user login and had valid auth token.
    string paths = request->getPath();

    std::string pathprefix = this->pathPrefix();
    std::size_t pos = paths.find(pathprefix);

    // pos has to equial the begin of the path, or error.
    pos += pathprefix.length();

    // find token to be deleted from path/url command, ignore "/"
    deltoken = paths.substr(pos+1); 
    
    it = m_db->auth_tokens.find(deltoken);
    if (it == m_db->auth_tokens.end() ) {
        // response->setStatus(404);
	throw ClientError::notFound();
        return;
    } 

    // a user can only delete its own auth-token session, not another user.
    if (it->second != u) {
	throw ClientError::forbidden();
        // response->setStatus(403);
        return;
    }

    // erase the deltoken from auth_tokens table.
    m_db->auth_tokens.erase(it);

    // XXX - if all auth_tokens for a user had been deleted, should the user be removed?
    // - need a reference count or have to walk through the auth-tokens database.

    return;

}
