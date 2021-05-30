#define RAPIDJSON_HAS_STDSTRING 1

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

int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

// we need a per user tokens to avoid tokens overwritten by different users.
string auth_token;
string user_id;

const string dcash_prompt ="D$> ";

//
// split cmdline input into string based on 
// - 1) whitespace(s) - 2) tab(s)
void split_string(const string &cmdline, vector <string> &cmds)
{
    int begin, cur;
    string token;
    int len = cmdline.length();

    begin = cur = 0;
    while (cur < len) {
        if (cmdline[cur] == ' ' || cmdline[cur] == '\t') {
	    if (cur != begin) {
		    token = cmdline.substr(begin, cur-begin);
		    cmds.push_back(token);
	    }
	    // ignore all consective whitespace or tabs
	    begin = cur = (cur+1);
         }
	 else {
	    cur++;
	 }
    }

    // the first or last substring
    if (cur != begin) {
        token = cmdline.substr(begin, cur-begin);
        cmds.push_back(token);
    }
}

// authenticate command handler
int process_auth_cmd(vector<string> &cmds)
{
    // cout << "process auth cmd" << endl;
    // two steps of interaction with Web-Server.
    // 1) HTTP POST
    // path="auth-tokens" body with user name and password.
    // expect to retrun auth-token and user id.
    // 2) HTTP PUT 
    // path=users with x-auth-token and user_id to update email
    // expect to return email and balance.
    // 3) if there are earlier auth-token we shall remove it.

    // XXX - do we need check valid cmds arguments and content like valid email address?

    HttpClient *client;
    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");
    HTTPClientResponse *response;
    string auth_path("/auth-tokens");
    string auth_body;

    // handling request body
    // the auth body contains username and password.
    WwwFormEncodedDict dict;
    dict.set("username", cmds[1]);
    dict.set("password", cmds[2]);
    auth_body = dict.encode();

    response = client->post(auth_path, auth_body);

    // check response
    if (!response->success()) {
        cout << "Error" << endl;
	delete response;
	return (-1);
    }

    cout << "succeed... for POST /auth-tokens " << endl;

    // 
    // https://rapidjson.org/md_doc_tutorial.html
    //
    Document *d = response->jsonBody();

    delete response;
    delete client;

    if ((!d->HasMember("auth_token")) ||
            (!d->HasMember("user_id"))) {
        cout << "Error" << endl;
        delete d;
	return (-1);
    }

    // obtain the auth_token and user id - it can have old auth token
    string new_auth_token = (*d)["auth_token"].GetString();
    string new_user_id = (*d)["user_id"].GetString();

    cout << "auth_token " << new_auth_token << endl;
    cout << "user_id " << new_user_id << endl;

    delete d;

    // - we need swith with different user, auth first to get current user id always.
    // - if return a different user id, that imply a differet auth/user or server bugs.
    // if (!user_id.empty() && new_user_id != user_id) {
    //    cout << "Error" << endl;
    //    return (-1);
    // }

    // start a new client and server connection

    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
    
    // we now need update our email address with account service.
    string users_path("/users");
    users_path = users_path +"/" + new_user_id;
    client->set_header("x-auth-token", new_auth_token);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");

    WwwFormEncodedDict dict2;
    dict2.set("email", cmds[3]);
    auth_body = dict2.encode();

    response = client->put(users_path, auth_body);
    delete client;
    
    // check response
    if (!response->success()) {
        cout << "Error" << endl;
	delete response;
	return (-1);
    }

    cout << "succeed...for PUT /users{user_id} " << endl;

    // 
    // https://rapidjson.org/md_doc_tutorial.html
    //
    d = response->jsonBody();
    if ((!d->HasMember("balance")) ||
            (!d->HasMember("email"))) {
        cout << "Error" << endl;
        delete d;
	delete response;
	return (-1);
    }

    int balance = (*d)["balance"].GetInt();
    float value = balance / 100;

    // use printf for formated print.
    printf("Balance: $%0.2f\n", value);

    delete d;
    delete response;

    // if we get a second new token, we shall del our old token on server.
    user_id = new_user_id;

    if (auth_token.empty()) {
        auth_token = new_auth_token;
        return 0;
    }

    // we need remove old auth token on the server for me or an earlier user.
    // start a new client and server connection

    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);
    
    // we now need update our email address with account service.
    auth_path = auth_path +"/" + auth_token;
    client->set_header("x-auth-token", auth_token);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");

    // update auth_token

    response = client->del(auth_path);
    delete client;

    if (!response->success()) {
        cout << "Error" << endl;
    } else {
        cout << "succeed... for DELETE /auth-tokens{auth_token} " << auth_token << endl;
    }

    auth_token = new_auth_token;
    delete response;
    return 0;
}

// balance command handler
int process_balance_cmd(vector<string> &cmds)
{
    cout << " - process balance cmd - " << endl;

    HttpClient *client;
    HTTPClientResponse *response;

    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);

    // we now need update our email address with account service.
    string users_path("/users");
    users_path = users_path +"/" + user_id;
    client->set_header("x-auth-token", auth_token);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");

    // use GET command to get balance.
    response = client->get(users_path);
    delete client;

    // check response
    if (!response->success()) {
        cout << "Error" << endl;
	delete response;
	return (-1);
    }

    cout << "succeed for (BALANCE).GET /users{user_id} " << user_id << endl;

    // 
    // https://rapidjson.org/md_doc_tutorial.html
    //
    Document *d = response->jsonBody();
    if ((!d->HasMember("balance")) ||
            (!d->HasMember("email"))) {
        cout << "Error" << endl;
        delete d;
	delete response;
	return (-1);
    }

    int balance = (*d)["balance"].GetInt();
    float value = balance / 100;

    // use printf for formated print.
    printf("Balance: $%0.2f\n", value);

    delete d;
    delete response;
    return 0;
}

//
// XXX - this shall work - use curl command worked.
//
HTTPClientResponse *get_token_from_stripe(vector<string> &cmds)
{
    HTTPClientResponse *response;

    // need SSL.
    HttpClient client("api.stripe.com", 443, true);
    client.set_header("Autherization", string("Bearer ") + PUBLISHABLE_KEY);
    client.set_header("Content-Type", "application/x-www-form-urlencoded");

    string token_body;

    // handling request body

    WwwFormEncodedDict dict;
    dict.set("card[number]", cmds[2]);
    dict.set("card[exp_month]", cmds[4]);
    dict.set("card[exp_year]", cmds[3]);
    dict.set("card[cvc]", cmds[5]);
    token_body = dict.encode();

    // send deposit request for token with stripe.
    response = client.post("/v1/tokens", token_body);
    return response;

}

// deposit command handler
int process_deposit_cmd(vector<string> &cmds)
{
    cout << "process deposit cmd" << endl;

    HttpClient *client;
    HTTPClientResponse *response;
    Document *d;
    string stripe_token = "tok_credit_card";

    // XXX - Step 1 - get a token id from stripe 
    // XXX - This step has problems with stripe.com...
    // XXX - comment out below when it works.
#ifdef STRIPE_WORKS_XXX

    response = get_token_from_stripe(cmds);

    // check response for tokenization
    if (!response->success()) {
        cout << "Error" << endl;
        return (-1);
    }
  
    d = response->jsonBody();
    string stripe_token = (*d)["id"].GetString();
    delete response;
    delete d;

#endif

    cout << "succeed for (STRIPE TOKEN) " << stripe_token << endl;
    
    //
    // Step 2 - send the deposit and charge id to the server
    //
    // deposit is saved as cents on server instead of dollar unit.
    float value = stof(cmds[1]);
    value *= 100;
    int amount = value; 

    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);

    // we now need update our email address with account service.
    string deposit_path("/deposits");
    client->set_header("x-auth-token", auth_token);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");

    string deposit_body;

    // handling request body
    // depost body contains amount and stripe token fields.
    WwwFormEncodedDict dict;
    dict.set("amount", amount);
    dict.set("stripe_token", stripe_token);
    deposit_body = dict.encode();

    // send depost request.
    response = client->post(deposit_path, deposit_body);
    delete client;

    // check response
    if (!response->success()) {
        cout << "Error" << endl;
	return (-1);
    }

    cout << "succeed for (DEPOSIT) " << auth_token << endl;
    // 
    // https://rapidjson.org/md_doc_tutorial.html
    //
    d = response->jsonBody();
    if (!d->HasMember("balance")) {
        cout << "Error" << endl;
        delete d;
	delete response;
	return (-1);
    }

    // There is a list of deposit history returned. we only display Balance field.
    int balance = (*d)["balance"].GetInt();
    value = balance / 100;

    // use printf for formated print.
    printf("Balance: $%0.2f\n", value);

    delete d;
    delete response;
    return 0;
}

// send command handler
int process_send_cmd(vector<string> &cmds)
{
    cout << "process send cmd" << endl;

    HttpClient *client;
    HTTPClientResponse *response;

    client = new HttpClient(API_SERVER_HOST.c_str(), API_SERVER_PORT);

    // we now need update our email address with account service.
    string trans_path("/transfers");
    string trans_body;
    client->set_header("x-auth-token", auth_token);
    client->set_header("Content-Type", "application/x-www-form-urlencoded");

    // transfer amount is saved as cents on server.
    float value = stof(cmds[2]);
    value *= 100;
    int amount = value; 

    // handling request body
    // depost body contains amount and stripe token fields.
    WwwFormEncodedDict dict;
    dict.set("to", cmds[1]);
    dict.set("amount", amount);
    trans_body = dict.encode();

    // send transfers post request.
    response = client->post(trans_path, trans_body);
    
    delete client;

    // check response
    if (!response->success()) {
        cout << "Error" << endl;
	delete response;
	return (-1);
    }

    cout << "succeed for (Transfer CMD) " << auth_token << endl;

    // 
    // https://rapidjson.org/md_doc_tutorial.html
    //
    Document *d = response->jsonBody();
    if (!d->HasMember("balance")) {
        cout << "Error" << endl;
        delete d;
	delete response;
	return (-1);
    }

    // There is a list of transfer history returned. we only display Balance field.
    int balance = (*d)["balance"].GetInt();
    value = balance / 100;

    // use printf for formated print.
    printf("Balance: $%0.2f\n", value);

    delete d;	
    delete response;
    return 0;
}

// logout command handler
int process_logout_cmd(vector<string> &cmds)
{
    // cout << "process logout cmd" << endl;
    // imply logout.
    return -2;
}


// all command handler list and processing table.
struct cmd_table_t {
    string cmd_name;
    int    cmd_args;
    int (*cmd_func) (vector<string> &cmds);
};

struct cmd_table_t cmd_tables[] = {
	{"auth", 	4, 	process_auth_cmd},
	{"balance", 	1,	process_balance_cmd},
	{"deposit",	6,	process_deposit_cmd},
	{"send",	3,	process_send_cmd},
	{"logout",	1,	process_logout_cmd}
};

// the main cmds process function
int process_cmds(vector<string> &cmds)
{
    int args = cmds.size();
    int len;
    int i;
    int ret = 0;

    len = sizeof(cmd_tables) / sizeof(struct cmd_table_t);

    for (i = 0; i < len; i++) {
        if (cmd_tables[i].cmd_name == cmds[0]) {
	    if (args != cmd_tables[i].cmd_args ) {
	        cout << "Error" << endl;
	        break;
	    }
	    
            ret = cmd_tables[i].cmd_func(cmds);
	    break;
	}
    }

    if (i == len) {
        // enter unknown command
	cout << "Error" << endl;
    }

    return ret;
}


int main(int argc, char *argv[]) {

  // get config.json content.
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0) {
    cout << "could not open config.json" << endl;
    exit(1);
  }

  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0) {
    config << string(buffer, ret);
  }
  
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

  int	inputFd = STDIN_FILENO;
  char 	inputLine[200], *p = inputLine;
  FILE 	*fp;
  bool 	batchMode = false;
  int  	nums;
  vector<string> cmds;
  size_t len = 150;

  // main start point - deal with batch mode or interactive mode.
  // exit if any error.
  if (argc == 2) {
      batchMode = true;
      inputFd = open(argv[1], O_RDONLY);
      if (inputFd < 0) {
	  cout << "open failed " << argv[1] << endl;
          exit(1);
      }
  } else {
      if (argc != 1) {
	  cout << "Incorrect command line arguments " << endl;
          exit(1);
      }
  }

  fp = fdopen(inputFd, "r");

  // enter main loop of read commands and process them
  while (true) {
      if (batchMode == false)
          cout << dcash_prompt;

       if (( nums = getline(&p, &len, fp)) == -1) {
	  // end of file or failure
	  fclose(fp);
	  exit(0);
       }

       // replace last '\n' character into '\0'
       inputLine[nums-1] = '\0';

       cmds.clear();
       split_string(inputLine, cmds);

       if (cmds.size() == 0 )
           continue;
       
       int ret = 0;
       if ((ret = process_cmds(cmds) == -2) ) {
           // logout command.
	   break;
       }
  }
  return 0;
}

