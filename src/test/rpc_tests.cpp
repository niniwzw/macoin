#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include "init.h" // for pwalletMain
#include "bitcoinrpc.h"
#include "ui_interface.h"
#include "base58.h"

using namespace std;
using namespace json_spirit;
using namespace boost;

Value CallRPC(string args)
{
    vector<string> vArgs;
    boost::split(vArgs, args, boost::is_any_of(" \t"));
    string strMethod = vArgs[0];
    vArgs.erase(vArgs.begin());
    Array params = RPCConvertValues(strMethod, vArgs);

    rpcfn_type method = tableRPC[strMethod]->actor;
    try {
        Value result = (*method)(params, false);
        return result;
    }
    catch (Object& objError)
    {
        throw runtime_error(find_value(objError, "message").get_str());
    }
}

std::string static EncodeDumpTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime);
}

std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    BOOST_FOREACH(unsigned char c, str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
        } else {
            ret << c;
        }
    }
    return ret.str();
}

BOOST_AUTO_TEST_SUITE(rpc_tests)

BOOST_AUTO_TEST_CASE(getinfo)
{
	Value info = CallRPC("getinfo");
	BOOST_CHECK(info.type() == obj_type);
}

BOOST_AUTO_TEST_CASE(get_keys)
{
    EnsureWalletIsUnlocked();
    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    pwalletMain->GetAllReserveKeys(setKeyPool);
    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());
    // produce output
    cout << strprintf("# Wallet dump created by Macoin %s (%s)\n", CLIENT_BUILD.c_str(), CLIENT_DATE.c_str());
    cout << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()).c_str());
    cout << strprintf("# * Best block at time of backup was %i (%s),\n", nBestHeight, hashBestChain.ToString().c_str());
    cout << strprintf("#   mined on %s\n", EncodeDumpTime(pindexBest->nTime).c_str());
    cout << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = CBitcoinAddress(keyid).ToString();
        bool IsCompressed;

        CKey key;
        if (pwalletMain->GetKey(keyid, key)) {
            if (pwalletMain->mapAddressBook.count(keyid)) {
                CSecret secret = key.GetSecret(IsCompressed);
                cout << strprintf("%s %s label=%s # addr=%s\n", CBitcoinSecret(secret, IsCompressed).ToString().c_str(), strTime.c_str(), EncodeDumpString(pwalletMain->mapAddressBook[keyid]).c_str(), strAddr.c_str());
            } else if (setKeyPool.count(keyid)) {
                CSecret secret = key.GetSecret(IsCompressed);
                cout << strprintf("%s %s reserve=1 # addr=%s\n", CBitcoinSecret(secret, IsCompressed).ToString().c_str(), strTime.c_str(), strAddr.c_str());
            } else {
                CSecret secret = key.GetSecret(IsCompressed);
                cout << strprintf("%s %s change=1 # addr=%s\n", CBitcoinSecret(secret, IsCompressed).ToString().c_str(), strTime.c_str(), strAddr.c_str());
            }
        }
    }
    cout << "\n";
    cout << "# End of dump\n";
}

BOOST_AUTO_TEST_CASE(addmultisigaddress)
{
		Value addrvalue = CallRPC("getnewpubkey2");
		string pubkey = addrvalue.get_str();
        string password = "1";
		const Object addrinfo = CallRPC(string("validatepubkey ") + pubkey).get_obj();
		const string addr = find_value(addrinfo, "address").get_str();
        
		//如果钱包已经加密
		if (pwalletMain->IsLocked())
		{
			CallRPC(string("walletpassphrase ")+password+" 30");
		}
		Value key      = CallRPC(string("dumpprivkey ") + addr);
		string privkey = key.get_str();
		
		//生成salt
        uint256 hash1 = Hash(privkey.begin(), privkey.end());
		Object multiinfo ;
		cout << "addmultisigaddress salt: " << hash1.GetHex() << endl;
		cout << "privkey:" << privkey << endl;
		cout << "pubkey:" << pubkey << endl;
		try {
			multiinfo = Macoin::addmultisigaddress(pubkey, hash1.GetHex());
		}catch(...){
              cout << "addmultisigaddress network error" << endl;
		}
		BOOST_CHECK(find_value(multiinfo,  "error").type() == null_type);
		const string pubkey1 = "\"" + find_value(multiinfo, "pubkey1").get_str() + "\"";
		const string pubkey2 = "\"" + find_value(multiinfo, "pubkey2").get_str() + "\"";
		const string multiaddr = find_value(multiinfo, "addr").get_str();
		const Value multisigwallet = CallRPC(string("addmultisigaddress 2 ") + "["+pubkey1+","+pubkey2+"]" + "Real");
		BOOST_CHECK(multisigwallet.type() == str_type);
}

BOOST_AUTO_TEST_SUITE_END()
