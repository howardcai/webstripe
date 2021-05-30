#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") { }

HTTPClientResponse *get_stripe_charge_info(Database *db, string &token, string &amount)
{
   HTTPClientResponse *response;

    // need SSL.
    HttpClient client("api.stripe.com", 443, true);
    client.set_basic_auth(db->stripe_secret_key, "");
    client.set_header("Content-Type", "application/x-www-form-urlencoded");

    string charge_body;

    // handling request body

    WwwFormEncodedDict dict;
    dict.set("amount", amount);
    dict.set("source", token);
    dict.set("currency", "usd");
    dict.set("description", "Kevin Stripe Test");
    charge_body = dict.encode();

    // send to stripe for charge
    response = client.post("/v1/charges", charge_body);
    return response;


}

void DepositService::post(HTTPRequest *request, HTTPResponse *response) {

    User *u;
    string amount, stripe_token;
    string charge_id="CHG_Credit_card";

    u = getAuthenticatedUser(request);

    // obtain deposit information
    WwwFormEncodedDict dict = request->formEncodedBody();
    amount = dict.get("amount");
    stripe_token = dict.get("stripe_token");

    // XXX - Send Charge request to Stripe.
    // XXX - uncomment this portion when STRIPE Tokens works.
    // XXX - use curl command worked however with the same parameters.

#ifdef STRIPE_WORKS_XXX
    HTTPClientResponse *stripe_rsp;
    stripe_rsp = get_stripe_charge_info(m_db, stripe_token, amount);
    
    if (!stripe_rsp->success()) {
	delete stripe_rsp;
        throw ClientError::unauthorized();
    }
 
    // charge authroized.
    Document *d = stripe_rsp->jsonBody();
    charge_id = (*d)["id"].GetString();

    delete d;
    delete stripe_rsp;
#endif

    // - succeed with Stripe:
    // - update user's account balance and record and respond.
    int value;
    stringstream ss(amount);
    ss >> value;
    
    u->balance += value;
    // update depost history
    Deposit *dd = new Deposit;
    dd->to = u;
    dd->amount = value;
    dd->stripe_charge_id = charge_id;
    m_db->deposits.push_back(dd);

    // now generate response
    Document document;
    Document::AllocatorType& a = document.GetAllocator();

    Value o;
    o.SetObject();
    o.AddMember("balance", u->balance, a);

    Value array;
    array.SetArray();

    size_t i;
    for (i = 0; i<m_db->deposits.size(); i++) {

        if (m_db->deposits[i]->to == u) {
            Value to;
            to.SetObject();
            to.AddMember("to", u->username, a);
            to.AddMember("amount", m_db->deposits[i]->amount, a);
            to.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id, a);
            array.PushBack(to, a);
	}
    }

    // add the deposit array to the return object member
    o.AddMember("Deposits", array, a);

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

