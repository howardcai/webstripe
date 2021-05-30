#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") { }

void TransferService::post(HTTPRequest *request, HTTPResponse *response) {

    User *u;
    
    string amount, touser;

    u = getAuthenticatedUser(request);

    // need update user's email address
    WwwFormEncodedDict dict = request->formEncodedBody();
    amount = dict.get("amount");
    touser = dict.get("to");

    stringstream ss(amount);
    int value;
    ss >> value;

    // if user don't have sufficient balance.
    if (u->balance < value) {
        throw ClientError::badRequest();
    }

    // check touser existed.
    map <string, User *>::iterator it;
    it = m_db->users.find(touser);

    if (it == m_db->users.end()) {
        throw ClientError::notFound();
    }

    // update user's account balance and record if succeed.
    // locking in multi-thread situation for DB access.

    it->second->balance += value;
    u->balance -= value;

    cout << "Recent Transfer" << endl;
    cout << "From: " << u->username << " new-balance:" << u->balance << endl;
    cout << "To: " << it->second->username << " new-balance:" << it->second->balance << endl;

    Transfer *newTrans = new Transfer;
    newTrans->from = u;
    newTrans->to = it->second;
    newTrans->amount = value;

    // add it into transfer history
    m_db->transfers.push_back(newTrans);

    // now generate response
    Document document;
    Document::AllocatorType& a = document.GetAllocator();

    Value o;
    o.SetObject();
    o.AddMember("balance", u->balance, a);


    Value array;
    array.SetArray();

    for (size_t i = 0; i < m_db->transfers.size(); i++) {
	if (m_db->transfers[i]->from->username == u->username) {
            Value to;
            to.SetObject();
            to.AddMember("from", u->username, a);
            to.AddMember("to", m_db->transfers[i]->to->username, a);
            to.AddMember("amount", m_db->transfers[i]->amount, a);
            array.PushBack(to, a);
	}
    }

    // add the deposit array to the return object member
    o.AddMember("Transfers", array, a);

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

