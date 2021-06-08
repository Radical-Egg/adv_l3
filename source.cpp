#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <regex>
#include <sstream>
#include <map>
#include <bitset>
#include <deque>
#include <thread>
#include <iterator>
#include <mutex>
#include <iomanip>

using namespace std;

enum class FILE_TYPE { BASE, XML, CSV };
struct CSVObject;

/*
class to store product objects
*/
class Products {
private:
	string barcode;
	string name;
	float price;
public:
	void setBarcode(string b) { barcode = b; }
	void setName(string n) { name = n; }
	void setPrice(float p) { price = p; }

	string getName() { return name; }
	string getBarcode() { return barcode; }
	float getPrice() { return price; }

	Products() : barcode{""}, name{""}, price{0} {}
	Products(string b, string n, float p) : barcode{b}, name{n}, price{p} {}
	~Products() {}
};

/*
Abstract base class for readers
*/
class Reader {
protected:
	string id;
	FILE_TYPE type;
public:
	virtual void parse() = 0;
	virtual void printData() = 0;


	string getID() { return id; }
	FILE_TYPE getType() { return type; }

	Reader(string id, FILE_TYPE t) : id{ id }, type{ t } { }
	virtual ~Reader() {}
};


/*
XMLReader to read from XML files and return a vector 
*/
class XMLReader : public Reader {
private:
	vector<Products> xmlData;
public:
	void parse() {
		ifstream inFile;
		string line;
		auto_ptr<bool> check(new bool(false));
		auto_ptr<Products> prod(new Products());

		auto f = [this](string line, auto_ptr<bool> &withinBlock, auto_ptr<Products> &product) {
			regex name("<Name>(.*)<\/Name>");
			regex barcode("<Barcode>(.*)<\/Barcode>");
			regex price("<Price>(.*)<\/Price>");
			regex startPattern("<Product>[\s\S]*?");
			regex endPattern("<\/Product>[\s\S]*?");
			smatch matched;

			if (regex_search(line, matched, startPattern)) { *withinBlock = true; }
			else if (regex_search(line, matched, endPattern)) {
				xmlData.push_back(*product);
				*withinBlock = false;
			}
			else if (*withinBlock) { 
				if (regex_search(line, matched, name)) { product->setName(matched.str(1)); }
				else if (regex_search(line, matched, barcode)) { product->setBarcode(matched.str(1)); }
				else if (regex_search(line, matched, price)) { product->setPrice(stof(matched.str(1))); }
			}
		};

		inFile.open(id);
		if (inFile.is_open()) {
			while (getline(inFile, line)) {
				f(line, check, prod);
			}
		} else {
			cout << "unable to open file" << endl;
		}
	}
	void printData() {
		auto beg = xmlData.begin();
		auto end = xmlData.end();

		while (beg != end) {
			Products temp = *(beg);
			cout << temp.getName() << '\n' << temp.getBarcode() << '\n' << temp.getPrice() << endl;
			beg++;
		}
	}
	vector<Products> getData() { return xmlData; }
	
	XMLReader(string id, FILE_TYPE t) : Reader(id, t) { this->parse(); }
	~XMLReader() {}
};


/*
CSVReader to read CSV files and return a vector to process output
*/
class CSVReader : public Reader {
private:
	struct CSVObject {
		string cartNumber;
		vector<string> hexCodes;

		CSVObject() : cartNumber("") {}
		CSVObject(string c) : cartNumber(c) {}

		void setCartNumber(string s) { cartNumber = s; }
		void pushHexCode(string h) { hexCodes.push_back(h); }

		string getCartNumber() { return cartNumber; }
		vector<string> getHexCode() { return hexCodes; }

		void printHexCodes() {
			auto beg = hexCodes.begin();
			auto end = hexCodes.end();

			while (beg != end) {
				cout << *beg << endl;
				beg++;
			}
		}

		string operator () () { return cartNumber; }
		void operator () (string hexCode) { pushHexCode(hexCode); }
	};
	vector<CSVObject> csvData;
public:
	virtual void parse() { 
		fstream inFile;
		string line;
		auto_ptr<CSVObject> prod(new CSVObject());

		auto f = [this](string line, auto_ptr<CSVObject> &CSVObj) {
			regex cartNumber("Cart[0-9]{3}");
			auto_ptr<vector<string>> tempHex(new vector<string>());

			smatch matched;

			if (regex_search(line, matched, cartNumber)) { 
				CSVObj->setCartNumber(matched.str(0)); 
			}
			else {
				auto tokenize = [this, &tempHex](string line) {
					stringstream ss(line);
					string token;
					char delim = ',';
					while (getline(ss, token, delim)) {
						token = hexToBinConvert(token);
						tempHex->push_back(token);
					}
					ss.str("");
				};
				tokenize(line);
				CSVObj->hexCodes = *tempHex;
				csvData.push_back(*CSVObj);
			}
			
		};
		inFile.open(id);
		if (inFile.is_open()) {
			while (getline(inFile, line)) {
				f(line, prod);
			}
		}
		else {
			cout << "unable to open file" << endl;
		}
	}

	void printData() {
		auto beg = csvData.begin();
		auto end = csvData.end();

		while (beg != end) {
			CSVObject data = *(beg);
			cout << data.getCartNumber() << endl;

			data.printHexCodes();

			beg++;
		}
	}

	vector<CSVObject> getData() { return csvData; }

	vector<string> split(const string& str, int splitLength)
	{
		int num = str.length() / splitLength;
		vector<string> ret;

		string error = "Hex keys should be 5 sets of 3 hex... this one was not";

		try {
			for (auto i = 0; i < num; i++) {
				ret.push_back(str.substr(i * splitLength, splitLength));
			}
			if (str.length() % splitLength != 0)
			{
				
				throw error;
			}
		}
		catch(string error) {
			cout << "this item will not be found when scanned" << endl;
		}

		return ret;
	}

	string hexToBinConvert(const string& s) {
		
		string out;
		vector<string> tempBinaryPlate;
		vector<string> splitHex = split(s, 3);

		auto beg = splitHex.begin();
		auto end = splitHex.end();

		while (beg != end) {
			string temp = *(beg);

			for (auto i : temp) {
				uint8_t n;
				if (i <= '9' and i >= '0')
					n = i - '0';
				else
					n = 10 + i - 'A';
				for (int8_t j = 3; j >= 0; --j)
					out.push_back((n & (1 << j)) ? '1' : '0');
			}
			out.erase(0, 3);
			tempBinaryPlate.push_back(out);
			out = "";
			beg++;
		}

		for (auto& elem : tempBinaryPlate) { out += elem; }

		return out;
	}
	CSVReader(string id, FILE_TYPE t) : Reader(id, t) { this->parse(); }
	~CSVReader() {}
};


/*
Abstract factory to create reader factorys
*/
class AbstractReaderFactory {
public:
	virtual Reader* createXML(string id, FILE_TYPE type) = 0;
	virtual Reader* createCSV(string id, FILE_TYPE type) = 0;
};

/*
Reader factory to make different types of readers
*/
class ReaderFactory : public AbstractReaderFactory {
public:
	Reader* createXML(string id, FILE_TYPE type) {
		return new XMLReader(id, type);
	}
	Reader* createCSV(string id, FILE_TYPE type) {
		return new CSVReader(id, type);
	}
};

/*
I used this class to interact with the reader factories
*/
class ClientReader {
public:
	AbstractReaderFactory* factory;
	ClientReader(AbstractReaderFactory* factory) : factory(factory) {};
};

/*
Product database from XMLReader
*/
class ProductDB {
private:
	string id;
	FILE_TYPE db_type;
	ClientReader* reader = new ClientReader(new ReaderFactory());
	Reader* reader_data;

	map<string, Products> database;

public:
	ProductDB(string i, FILE_TYPE type) : id(i), db_type(type) { this->loadDB(); }
	~ProductDB() {
		delete reader;
		delete reader_data;
	}
	void showData() { 
		reader_data->printData(); }

	/*
	Purpose: load data base with XML file data
	*/
	void loadDB() {
		// creating XML reader with factory 
		reader_data = static_cast<XMLReader*>(reader->factory->createXML(id, db_type));
		vector<Products> productsList = static_cast<XMLReader*>(reader_data)->getData();

		auto beg = productsList.begin();
		auto end = productsList.end();

		while (beg != end) {
			Products temp = *(beg);
			string tempBarcode = temp.getBarcode();

			database.insert(pair<string, Products>(tempBarcode, temp));
			beg++;
		}
	}
	void printDB() {
		for (auto elem : database) {
			cout << "Barcode: "<< elem.first << endl;
			cout << elem.second.getName() << endl;
			cout << elem.second.getPrice() << endl;
		}
	}
	bool search(string key) {
		auto searchFor = database.find(key);

		if (searchFor != database.end()) {
			return true;
		}
		else { return false; }
	}
	Products* retriveEntry(const string key) {
		if (search(key)) {
			return &database[key];
		}
		else { return nullptr; }
	}
};


/*
Cart class used to create objects from dcsv data
*/
class Cart {
private:
	pair <string, vector<string>> itemList;
	string receipt;
	float running_total = 0;
public:
	pair<string, vector<string>> getCartData() { return itemList; }
	void updateCartName(string name) { itemList.first = name; }
	void pushItem(string item) { itemList.second.push_back(item); }
	void printCart() {
		int counter = 0;
		cout << itemList.first << endl;
		auto beg = itemList.second.begin();
		auto end = itemList.second.end();


		while (beg != end) {
			cout << *beg << endl;
			beg++;
		}
	}
	
	string getCartName() { return itemList.first; }
	vector<string> getHexList() { return itemList.second; }
	string getReceipt() { return receipt; }
	
	// update receipt for cart
	void updateReceipt(Products* product) {
		stringstream ss;
		stringstream total;

		ss << fixed << setprecision(2) << product->getPrice();
		string entry = "Item: " + product->getName() + " - "+"Price: " + ss.str() + '\n';
		
		running_total += product->getPrice();
		receipt += entry;		
	}
	// apply running total to end of receipt
	void applyRunningTotal() {
		stringstream total;
		
		total << "Total: " << fixed << setprecision(2) << running_total << endl;

		receipt += total.str();
	}
};


/*
Creational class to create carts from an CSVReader object
*/
class CartCreator {
private:
	string id;
	FILE_TYPE db_type;
	vector<Cart> cartList;
	ClientReader* reader = new ClientReader(new ReaderFactory());
	Reader* reader_data;
public:
	CartCreator(string i, FILE_TYPE t) : id(i), db_type(t) { this->loadCarts(); }
	~CartCreator() { 
		delete reader;
		delete reader_data;
	}

	vector<Cart> getAllCarts() { return cartList; }
	void printMyCarts() {
		auto beg = cartList.begin();
		auto end = cartList.end();

		while (beg != end) {
			auto tempHomie = *(beg);

			tempHomie.printCart();

			beg++;
		}
	}
	void loadCarts() {
		reader_data = static_cast<CSVReader*>(reader->factory->createCSV(id, db_type));
		
		auto CSVCartList = static_cast<CSVReader*>(reader_data)->getData();

		auto beg = CSVCartList.begin();
		auto end = CSVCartList.end();

		while (beg != end) {
			Cart* tempCart = new Cart();
			auto tempCSVObject = *(beg);

			tempCart->updateCartName(tempCSVObject.getCartNumber());

			auto hexList = tempCSVObject.getHexCode();

			vector<string>::iterator hexStart = hexList.begin();
			vector<string>::iterator hexEnd = hexList.end();

			while (hexStart != hexEnd) {
				string hexItem = *(hexStart);
				tempCart->pushItem(hexItem);
				hexStart++;
			}

			cartList.push_back(*tempCart);
			delete tempCart;
	
			beg++;
		}
	}
	int size() { return cartList.size(); }
	void printCartReceipts() {
		for (auto& cart : cartList) {
			cout << cart.getReceipt() << endl;
			system("pause");
		}
	}
};

/*
Queue manager class used to separate carts into lanes and process the data using
threading
*/
class QueueManager {
private:
	deque<vector<Cart>> lanes;
	ProductDB* database;
	CartCreator* carts;
	mutex m;
	condition_variable cv;
	bool ready = false;
	bool processed = false;
	int maxLanes = 14;
public:
	QueueManager(ProductDB* prodDB, CartCreator* cartDB) {
		database = prodDB;
		carts = cartDB;
		this->distributeTraffic();
		this->threadCarts();
	}

	void distributeTraffic() {
		vector<Cart> allCarts = carts->getAllCarts();

		int cartsPerLane = carts->size() / maxLanes;

		vector<Cart>* tempCartHolder = new vector<Cart>();
		for (auto& cart : allCarts) {
			tempCartHolder->push_back(cart);
			
			if (tempCartHolder->size() == cartsPerLane) {
				lanes.push_front(*tempCartHolder);
				delete tempCartHolder;
				tempCartHolder = new vector<Cart>();
			}
			
		} // lane 15 for overflow
		if (tempCartHolder->size() > 0) {
			lanes.push_front(*tempCartHolder);
			delete tempCartHolder;
		}
	}
	void threadCarts() {
		auto worker = [this](string barcode, Cart* working_cart) {
			unique_lock<mutex> lk(m);
			cv.wait(lk, [this] {return ready; });

			Products* temp = this->scan(barcode);

			if (temp != nullptr) {
				working_cart->updateReceipt(temp);
			}

			processed = true;
			lk.unlock();
			cv.notify_one();
		};

		for (auto& lane : lanes) {
			for (auto& letsDoThis : lane) {
				for (auto& barcode : letsDoThis.getHexList()) {
					thread th_thread(worker, barcode, &letsDoThis);
					{
						lock_guard<mutex> lk(m);
						ready = true;
					}

					cv.notify_one();
					{
						unique_lock<mutex> lk(m);
						cv.wait(lk, [this] {return processed; });
					}
					th_thread.join();
				}
			}
		}
	}
	void printAllReceipts() {
		for (auto& lane : lanes) {
			for (auto& cart : lane) {
				cart.applyRunningTotal();
				cout << cart.getCartName() << endl;
				cout << cart.getReceipt() << endl;
			}
		}
	}
	Products* scan(string key) {
		string errorMsg = "Unable to scan this product";
		try {
			Products* temp = new Products();
			if ((temp = database->retriveEntry(key)) != nullptr) {
				return temp;
			}
			else { throw(errorMsg); }
		}
		catch(string errorMsg) {
			// would log an error message here but for now will leave it empty to
			// keep output readable
			return nullptr;
		}
	}
};
int main() {
	string db_file_name = "ProductPrice.xml";
	FILE_TYPE db_type = FILE_TYPE::XML;

	string cart_file_name = "Cart.csv";
	FILE_TYPE cart_db = FILE_TYPE::CSV;
	
	ProductDB* myDB = new ProductDB(db_file_name, db_type);
	CartCreator* allTheCarts = new CartCreator(cart_file_name, cart_db);

	QueueManager lanes(myDB, allTheCarts);

	lanes.printAllReceipts();

	return 0;
}
