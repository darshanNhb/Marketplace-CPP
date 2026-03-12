#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <ctime>

using namespace std;

class Utils
{
public:
    static int generate_id()
    {
        return rand() % 90000 + 10000;
    }

    static string get_time()
    {
        time_t now = time(0);
        char *dt = ctime(&now);
        string s(dt);
        if (!s.empty() && s.back() == '\n')
            s.pop_back();
        return s;
    }

    static void clear_screen()
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
    }
};

struct Review
{
    string buyerId;
    int rating;
    string comment;

    string serialize() const
    {
        return buyerId + "|" + to_string(rating) + "|" + comment;
    }

    static Review deserialize(const string &data)
    {
        stringstream ss(data);
        string bId, rat, com;
        getline(ss, bId, '|');
        getline(ss, rat, '|');
        getline(ss, com, '|');
        return {bId, (rat == "" ? 0 : stoi(rat)), com};
    }
};

class Product
{
public:
    int id;
    string name;
    float price;
    int quantity;
    string category;
    string sellerId;

    bool isAuction;
    float currentHighBid;
    string highBidderId;
    bool isClosed;

    vector<Review> reviews;

    Product() : id(0), price(0), quantity(0), category("General"), isAuction(false), currentHighBid(0), isClosed(false) {}

    float get_avg_rating() const
    {
        if (reviews.empty())
            return 0.0;
        float sum = 0;
        for (auto &r : reviews)
            sum += r.rating;
        return sum / reviews.size();
    }

    void display(bool detailed = false) const
    {
        cout << "[" << id << "] " << left << setw(15) << name
             << " | " << setw(10) << category
             << " | Price: " << right << setw(7) << fixed << setprecision(2) << price
             << " | Qty: " << setw(3) << quantity;

        if (isAuction)
            cout << " [AUCTION]";

        float rating = get_avg_rating();
        if (rating > 0)
            cout << " | Rating: " << setprecision(1) << rating << "/5";
        cout << endl;

        if (detailed && !reviews.empty())
        {
            cout << "   Reviews:" << endl;
            for (auto &r : reviews)
            {
                cout << "   - [" << r.rating << "/5] " << r.buyerId << ": " << r.comment << endl;
            }
        }
    }
};

class Transaction
{
public:
    int id;
    string buyerId;
    string sellerId;
    int productId;
    int quantity;
    float totalPrice;
    string date;
    string type;

    void save_to_file()
    {
        ofstream fout("data/transactions.txt", ios::app);
        fout << id << "," << buyerId << "," << sellerId << "," << productId << "," << quantity << "," << totalPrice << "," << date << "," << type << endl;
    }
};

class User
{
protected:
    string userId;
    string name;
    string password;
    float walletBalance;
    vector<string> notifications;

public:
    User() : userId(""), name(""), password(""), walletBalance(0.0) {}
    virtual ~User() {}

    string get_id() const { return userId; }
    float get_balance() const { return walletBalance; }
    void add_balance(float amount) { walletBalance += amount; }
    bool deduct_balance(float amount)
    {
        if (walletBalance >= amount)
        {
            walletBalance -= amount;
            return true;
        }
        return false;
    }

    void add_notification(const string &msg)
    {
        notifications.push_back("[" + Utils::get_time() + "] " + msg);
    }

    void view_notifications()
    {
        cout << "\n--- NOTIFICATIONS ---" << endl;
        if (notifications.empty())
            cout << "No new alerts." << endl;
        for (auto &n : notifications)
            cout << n << endl;
        notifications.clear();
    }

    bool login(const string &id, const string &pass)
    {
        if (load_data(id))
        {
            if (password == pass)
                return true;
        }
        return false;
    }

    virtual void save_data()
    {
        ofstream fout("data/users/" + userId + ".txt");
        fout << userId << "\n"
             << name << "\n"
             << password << "\n"
             << walletBalance << endl;
        for (auto &n : notifications)
            fout << "NOTIF:" << n << endl;
    }

    virtual bool load_data(const string &id)
    {
        ifstream fin("data/users/" + id + ".txt");
        if (!fin)
            return false;
        getline(fin, userId);
        getline(fin, name);
        getline(fin, password);
        string balStr;
        getline(fin, balStr);
        walletBalance = (balStr == "" ? 0.0f : stof(balStr));

        notifications.clear();
        string line;
        while (getline(fin, line))
        {
            if (line.substr(0, 6) == "NOTIF:")
                notifications.push_back(line.substr(6));
        }
        return true;
    }

    virtual void menu(vector<Product> &marketplace) = 0;
};

class Seller : public User
{
    vector<Product> myProducts;

public:
    void load_inventory()
    {
        myProducts.clear();
        ifstream fin("data/inventory_" + userId + ".txt");
        if (!fin)
            return;
        string line;
        while (getline(fin, line))
        {
            if (line.empty())
                continue;
            stringstream ss(line);
            Product p;
            string temp;
            getline(ss, temp, ',');
            p.id = stoi(temp);
            getline(ss, p.name, ',');
            getline(ss, temp, ',');
            p.price = stof(temp);
            getline(ss, temp, ',');
            p.quantity = stoi(temp);
            getline(ss, p.category, ',');
            getline(ss, temp, ',');
            p.isAuction = (temp == "1");
            getline(ss, temp, ',');
            p.currentHighBid = stof(temp);
            getline(ss, p.highBidderId, ',');
            getline(ss, temp, ',');
            p.isClosed = (temp == "1");

            string revs;
            if (getline(ss, revs))
            {
                stringstream ssr(revs);
                string r;
                while (getline(ssr, r, ';'))
                {
                    if (!r.empty())
                        p.reviews.push_back(Review::deserialize(r));
                }
            }
            p.sellerId = userId;
            myProducts.push_back(p);
        }
    }

    void save_inventory()
    {
        ofstream fout("data/inventory_" + userId + ".txt");
        for (auto &p : myProducts)
        {
            fout << p.id << "," << p.name << "," << p.price << "," << p.quantity << ","
                 << p.category << "," << (p.isAuction ? "1" : "0") << ","
                 << p.currentHighBid << "," << p.highBidderId << "," << (p.isClosed ? "1" : "0") << ",";
            for (auto &r : p.reviews)
                fout << r.serialize() << ";";
            fout << endl;
        }
    }

    void add_product()
    {
        Product p;
        p.id = Utils::generate_id();
        p.sellerId = userId;
        cout << "Enter Name: ";
        getline(cin, p.name);
        cout << "Enter Price: ";
        cin >> p.price;
        cout << "Enter Quantity: ";
        cin >> p.quantity;
        cout << "Enter Category: ";
        cin.ignore();
        getline(cin, p.category);
        cout << "Is this an Auction? (1=Yes, 0=No): ";
        int choice;
        cin >> choice;
        p.isAuction = (choice == 1);
        if (p.isAuction)
            p.currentHighBid = p.price;

        myProducts.push_back(p);
        save_inventory();
        cout << "Product Added!" << endl;
    }

    void view_sales_report()
    {
        ifstream fin("data/transactions.txt");
        if (!fin)
        {
            cout << "No sales." << endl;
            return;
        }
        cout << "\n--- SALES REPORT ---" << endl;
        string line;
        float totalIncome = 0;
        while (getline(fin, line))
        {
            stringstream ss(line);
            string id, bid, sid, pid, qty, price, date, type;
            getline(ss, id, ',');
            getline(ss, bid, ',');
            getline(ss, sid, ',');
            if (sid == userId)
            {
                getline(ss, pid, ',');
                getline(ss, qty, ',');
                getline(ss, price, ',');
                getline(ss, date, ',');
                getline(ss, type, ',');
                cout << date << " | Order #" << id << " | Product #" << pid << " | Qty: " << qty << " | Amount: $" << price << endl;
                totalIncome += stof(price);
            }
        }
        cout << "Total Revenue: $" << fixed << setprecision(2) << totalIncome << endl;
    }

    void menu(vector<Product> &marketplace) override
    {
        load_inventory();
        int choice;
        do
        {
            cout << "\n--- SELLER DASHBOARD (" << name << ") ---" << endl;
            cout << "Wallet: $" << walletBalance << endl;
            cout << "1. View Inventory\n2. Add Product\n3. Sales Reports\n4. Notifications\n0. Logout\nChoice: ";
            cin >> choice;
            cin.ignore();
            switch (choice)
            {
            case 1:
                for (auto &p : myProducts)
                {
                    p.display();
                    if (p.quantity <= 5)
                        cout << "   [WARNING: LOW STOCK!]" << endl;
                }
                break;
            case 2:
                add_product();
                break;
            case 3:
                view_sales_report();
                break;
            case 4:
                view_notifications();
                save_data();
                break;
            }
        } while (choice != 0);
    }
};

class Buyer : public User
{
    struct CartItem
    {
        int pid;
        int qty;
        string sid;
        float price;
        string name;
    };
    vector<CartItem> cart;

public:
    void view_marketplace(vector<Product> &marketplace)
    {
        cout << "\n--- MARKETPLACE ---" << endl;
        for (auto &p : marketplace)
        {
            if (p.quantity > 0 || p.isAuction)
                p.display();
        }
    }

    void checkout(vector<Product> &marketplace)
    {
        if (cart.empty())
        {
            cout << "Cart empty." << endl;
            return;
        }
        float subtotal = 0;
        for (auto &item : cart)
            subtotal += item.price * item.qty;

        cout << "Total to Pay: $" << subtotal << endl;
        if (walletBalance < subtotal)
        {
            cout << "Insufficient funds!" << endl;
            return;
        }

        deduct_balance(subtotal);
        int tid = Utils::generate_id();
        string date = Utils::get_time();

        for (auto &item : cart)
        {
            Transaction t = {tid, userId, item.sid, item.pid, item.qty, item.price * item.qty, date, "SALE"};
            t.save_to_file();
            for (auto &mp : marketplace)
            {
                if (mp.id == item.pid)
                    mp.quantity -= item.qty;
            }
        }
        cart.clear();
        save_data();
        cout << "Checkout Success! ID: " << tid << endl;
    }

    void place_bid(vector<Product> &marketplace)
    {
        int id;
        float bid;
        cout << "Enter Auction ID: ";
        cin >> id;
        for (auto &p : marketplace)
        {
            if (p.id == id && p.isAuction)
            {
                cout << "Current Bid: $" << p.currentHighBid << " by " << p.highBidderId << endl;
                cout << "Your bid: ";
                cin >> bid;
                if (bid > p.currentHighBid && walletBalance >= bid)
                {
                    p.currentHighBid = bid;
                    p.highBidderId = userId;
                    cout << "Bid placed!" << endl;
                }
                else
                    cout << "Invalid bid." << endl;
                return;
            }
        }
    }

    void menu(vector<Product> &marketplace) override
    {
        int choice;
        do
        {
            cout << "\n--- BUYER DASHBOARD (" << name << ") ---" << endl;
            cout << "Wallet: $" << walletBalance << " | Cart: " << cart.size() << endl;
            cout << "1. Browse\n2. Add to Cart\n3. Checkout\n4. Bid\n5. Wallet Top-up\n6. Notifications\n0. Logout\nChoice: ";
            cin >> choice;
            cin.ignore();
            switch (choice)
            {
            case 1:
                view_marketplace(marketplace);
                break;
            case 2:
            {
                int id, q;
                cout << "Product ID: ";
                cin >> id;
                for (auto &mp : marketplace)
                {
                    if (mp.id == id)
                    {
                        cout << "Qty: ";
                        cin >> q;
                        if (q <= mp.quantity)
                            cart.push_back({mp.id, q, mp.sellerId, mp.price, mp.name});
                        break;
                    }
                }
                break;
            }
            case 3:
                checkout(marketplace);
                break;
            case 4:
                place_bid(marketplace);
                break;
            case 5:
            {
                float amt;
                cout << "Amount: ";
                cin >> amt;
                add_balance(amt);
                save_data();
                break;
            }
            case 6:
                view_notifications();
                save_data();
                break;
            }
        } while (choice != 0);
    }
};

class Admin : public User
{
public:
    Admin()
    {
        userId = "admin";
        name = "Admin";
        password = "as";
    }
    void menu(vector<Product> &marketplace) override
    {
        int choice;
        do
        {
            cout << "\n--- ADMIN CONSOLE ---" << endl;
            cout << "1. View All Transactions\n2. CSV Export\n0. Logout\nChoice: ";
            cin >> choice;
            cin.ignore();
            if (choice == 1)
            {
                ifstream fin("data/transactions.txt");
                string l;
                while (getline(fin, l))
                    cout << l << endl;
            }
            else if (choice == 2)
            {
                cout << "Exporting to data/report.csv..." << endl;
                ifstream fin("data/transactions.txt");
                ofstream fout("data/report.csv");
                fout << "TID,Buyer,Seller,PID,Qty,Price,Date,Type\n";
                string l;
                while (getline(fin, l))
                    fout << l << "\n";
            }
        } while (choice != 0);
    }
};

class SystemManager
{
    vector<Product> marketplace;
    Admin admin;

public:
    void init()
    {
        system("mkdir data 2>nul");
        system("mkdir data\\users 2>nul");
        ifstream fin("data/sellers_list.txt");
        string sid;
        while (fin >> sid)
        {
            ifstream pin("data/inventory_" + sid + ".txt");
            string line;
            while (getline(pin, line))
            {
                if (line.empty())
                    continue;
                stringstream ss(line);
                Product p;
                string t;
                getline(ss, t, ',');
                p.id = stoi(t);
                getline(ss, p.name, ',');
                getline(ss, t, ',');
                p.price = stof(t);
                getline(ss, t, ',');
                p.quantity = stoi(t);
                getline(ss, p.category, ',');
                getline(ss, t, ',');
                p.isAuction = (t == "1");
                getline(ss, t, ',');
                p.currentHighBid = stof(t);
                getline(ss, p.highBidderId, ',');
                getline(ss, t, ',');
                p.isClosed = (t == "1");
                p.sellerId = sid;
                marketplace.push_back(p);
            }
        }
    }

    void run()
    {
        init();
        while (true)
        {
            cout << "\n--- MARKETPLACE ---" << endl;
            cout << "1. Seller\n2. Buyer\n3. Admin\n0. Exit\nChoice: ";
            int t;
            cin >> t;
            if (t == 0)
                break;
            if (t == 3)
            {
                string p;
                cout << "Pass: ";
                cin >> p;
                if (p == "as")
                    admin.menu(marketplace);
                continue;
            }
            cout << "1. Login\n2. Signup\nChoice: ";
            int a;
            cin >> a;
            if (t == 1)
            {
                Seller s;
                if (a == 2)
                {
                    cout << "ID: ";
                    string i;
                    cin >> i;
                    cout << "Name: ";
                    string n;
                    cin.ignore();
                    getline(cin, n);
                    cout << "Pass: ";
                    string ps;
                    cin >> ps;
                    ofstream f("data/users/" + i + ".txt");
                    f << i << "\n"
                      << n << "\n"
                      << ps << "\n0\n";
                    ofstream fl("data/sellers_list.txt", ios::app);
                    fl << i << "\n";
                    cout << "Signed up!";
                }
                else
                {
                    string i, p;
                    cout << "ID: ";
                    cin >> i;
                    cout << "Pass: ";
                    cin >> p;
                    if (s.login(i, p))
                        s.menu(marketplace);
                }
            }
            else if (t == 2)
            {
                Buyer b;
                if (a == 2)
                {
                    cout << "ID: ";
                    string i;
                    cin >> i;
                    cout << "Name: ";
                    string n;
                    cin.ignore();
                    getline(cin, n);
                    cout << "Pass: ";
                    string ps;
                    cin >> ps;
                    ofstream f("data/users/" + i + ".txt");
                    f << i << "\n"
                      << n << "\n"
                      << ps << "\n0\n";
                    ofstream fl("data/buyers_list.txt", ios::app);
                    fl << i << "\n";
                    cout << "Signed up!";
                }
                else
                {
                    string i, p;
                    cout << "ID: ";
                    cin >> i;
                    cout << "Pass: ";
                    cin >> p;
                    if (b.login(i, p))
                        b.menu(marketplace);
                }
            }
        }
    }
};

int main()
{
    srand(time(0));
    SystemManager sys;
    sys.run();
    return 0;
}
