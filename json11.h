/*
 * This file is part of json11 project (https://github.com/borisgontar/json11).
 *
 * Copyright (c) 2013 Boris Gontar.
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

// Version 0.6.5, 2013-11-07

#ifndef JSON11_H_
#define JSON11_H_

#if defined(_MSC_VER)
	#pragma warning( push )
	#pragma warning( disable:4244 )	// Disables warning 'conversion from '*' to '**', possible loss of data'
#endif

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <regex>
#include <cfloat>
#include <stdexcept>
#include <initializer_list>

class Json {
public:
    enum Type {
        JSNULL, BOOL, NUMBER, STRING, ARRAY, OBJECT
    };
private:
    struct Schema;  // forward dcl
    struct Node {
    	unsigned refcnt;
        Node(unsigned init = 0);
        virtual ~Node();
        virtual Type type() const { return Type::JSNULL; }
        virtual void print(std::ostream& out) const { out << "null"; }
        virtual void traverse(void (*f)(const Node*)) const { f(this); }
        virtual bool contains(const Node* /*that*/) const { return false; }
        virtual bool operator == (const Node& that) const { return this == &that; }
        virtual bool is_schema() const { return false; }
        void unref();
#ifdef WITH_SCHEMA
        virtual void validate(const Schema& schema, std::vector<const Node*>&) const;
#endif
#ifdef TEST
        static std::vector<Node*> nodes;
        static void test();
#endif
        static Node null, undefined;
    };
    //
    struct Bool : Node {
        Bool(bool /*x*/) { refcnt = 1; }
        Type type() const override { return Type::BOOL; }
        void print(std::ostream& out) const override;
        static Bool T;
        static Bool F;
    };
    //
    struct Number : Node {
        long double value;
        int prec;
        Number(long double x) { prec = LDBL_DIG; value = x; }
        Number(double x) { prec = DBL_DIG; value = x; }
        Number(float x) { prec = FLT_DIG; value = x; }
        Number(long long x) { prec = DBL_DIG; value = x; }
        Number(long x) { prec = -1; value = x; }
        Number(int x) { prec = -1; value = x; }
        Number(unsigned long long x) { prec = DBL_DIG; value = x; }
        Number(unsigned long x) { prec = -1; value = x; }
        Number(unsigned int x) { prec = -1; value = x; }
        Number(std::istream&);
        Type type() const override { return Type::NUMBER; }
        void print(std::ostream& out) const override;
        bool operator == (const Node& that) const override;
#ifdef WITH_SCHEMA
        void validate(const Schema& schema, std::vector<const Node*>&) const override;
#endif
    };
    //
    struct String : Node {
        std::string value;
        String(std::string s) { value = s; }
        String(std::istream&);
        Type type() const override { return Type::STRING; }
        void print(std::ostream& out) const override;
        bool operator == (const Node& that) const override;
#ifdef WITH_SCHEMA
        void validate(const Schema& schema, std::vector<const Node*>&) const override;
#endif
    };
    //
    struct Array : Node {
        std::vector<Node*> list;
        virtual ~Array();
        Type type() const override { return Type::ARRAY; }
        void print(std::ostream&) const override;
        void traverse(void (*f)(const Node*)) const override;
        void add(Node*);
        void ins(int, Node*);
        void del(int);
        void repl(int, Node*);
        bool contains(const Node*) const override;
        bool operator == (const Node& that) const override;
#ifdef WITH_SCHEMA
        void validate(const Schema& schema, std::vector<const Node*>&) const override;
#endif
    };
    //
    struct Object : Node {
        std::map<const std::string*, Node*> map;
        virtual ~Object();
        Type type() const override { return Type::OBJECT; }
        void print(std::ostream&) const override;
        void traverse(void (*f)(const Node*)) const override;
        Node* get(const std::string&) const;
        void set(const std::string&, Node*);
        bool contains(const Node*) const override;
        bool operator == (const Node& that) const override;
#ifdef WITH_SCHEMA
        void validate(const Schema& schema, std::vector<const Node*>&) const override;
#endif
    };
    //
#ifdef WITH_SCHEMA
    struct Schema : Node {
        Schema(Node*);
        virtual ~Schema();
        std::string uri;
        std::string s_type;
        Array* s_enum = nullptr;
        std::vector<Schema*> allof;
        std::vector<Schema*> anyof;
        std::vector<Schema*> oneof;
        Schema* s_not = nullptr;
        long double max_num = LDBL_MAX;
        long double min_num = -LDBL_MAX;
        long double mult_of = 0;
        bool max_exc = false, min_exc = false;
        unsigned long max_len = UINT32_MAX;
        unsigned long min_len = 0;
        std::regex* pattern = nullptr;  // regex
        Schema* item = nullptr;
        std::vector<Schema*> items;
        Schema* add_items = nullptr;
        bool add_items_bool = false;
        bool unique_items = false;
        Object* props = nullptr;
        Object* pat_props = nullptr;
        Schema* add_props = nullptr;
        bool add_props_bool = false;
        Array* required = nullptr;
        Object* deps = nullptr;
        Object* defs = nullptr;
        Node* deflt = nullptr;
        bool is_schema() const { return true; }
    };
#endif
    //
    class Property {
        Node* host;
        std::string key;
        int index;
        Json target() const;
    public:
        Property(Node*, const std::string&);
        Property(Node*, int);
        operator Json() const { return target(); }
        explicit operator bool() const { return static_cast< bool >( target() ); }
        operator int() const { return target(); }
        operator long() const { return target(); }
        operator long long() const { return target(); }
        operator unsigned int() const { return target(); }
        operator unsigned long() const { return target(); }
        operator unsigned long long() const { return target(); }
        operator float() const { return target(); }
        operator double() const { return target(); }
        operator long double() const { return target(); }
        operator std::string() const { return target(); }
        Property operator [] (const std::string& k) { return target()[k]; }
        Property operator [] (const char* k) { return (*this)[std::string(k)]; }
        Property operator [] (int i) {return target()[i]; }
        const Property operator [] (int i) const {return target()[i]; }

        Json operator = (const Json&);
        Json operator = (const Property&);
        bool operator == (const Json& js) const { return (Json)(*this) == js; }
        bool operator != (const Json& js) const { return !(*this == js); }
        std::vector<std::string> keys() const { return target().keys(); }
        bool has(const std::string& key) const { return target().has(key); }
        friend std::ostream& operator << (std::ostream& out, const Property& p) {
            return out << (Json)p;
        }
        friend Json;
    };
    Array* mkarray();
    Object* mkobject();
    static std::set<std::string> keyset;   // all propery names
    static int level;   // for pretty printing
    //
    Json(Node* node) {
        (root = (node == nullptr ? &Node::null : node))->refcnt++;
    }
    Node* root;
    //
public:
    // constructors
    Json() { (root = &Node::null)->refcnt++; }
    Json(const Json& that);
    Json(Json&& that);
    Json(std::istream&, bool full = true);   // parse
    virtual ~Json();
    //
    // initializers
    Json& operator = (const Json&);
    Json& operator = (Json&&);
    //
    // more constructors
    Json(bool x) { (root = (x ? &Bool::T : &Bool::F))->refcnt++; }
    Json(int x) { (root = new Number(x))->refcnt++; }
    Json(long x) { (root = new Number(x))->refcnt++; }
    Json(long long x) { (root = new Number(x))->refcnt++; }
    Json(unsigned int x) { (root = new Number(x))->refcnt++; }
    Json(unsigned long x) { (root = new Number(x))->refcnt++; }
    Json(unsigned long long x) { (root = new Number(x))->refcnt++; }
    Json(float x) { (root = new Number(x))->refcnt++; }
    Json(double x) { (root = new Number(x))->refcnt++; }
    Json(long double x) { (root = new Number(x))->refcnt++; }
    Json(const std::string& s) { (root = new String(s))->refcnt++; }
    Json(const char* s) { (root = new String(s))->refcnt++; }
    Json(std::initializer_list<Json>);
    explicit Json(const Property& p) { (root = p.target().root)->refcnt++; }
    //
    // casts
    Type type() const { return root->type(); }
    explicit operator bool() const;
    operator int() const;
    operator long() const;
    operator long long() const;
    operator unsigned int() const;
    operator unsigned long() const;
    operator unsigned long long() const;
    operator float() const;
    operator double() const;
    operator long double() const;
    operator std::string() const;
    //
    // object
    Json& set(std::string key, const Json& val);
    Json get(const std::string& key) const;
    bool has(const std::string& key) const;
    std::vector<std::string> keys() const;
    //
    // array
    Json& operator << (const Json&);
    void insert(int index, const Json&);
    void erase(int index);
    Json& replace(int index, const Json&);
    //
    // subscript
    int size() const;
    Json::Property operator [] (const std::string&);
    Json::Property operator [] (const char* k) { return (*this)[std::string(k)]; }
    Json::Property operator [] (int);
    const Json::Property operator [] (int) const;
    //
    // stringify
    std::string stringify() const { return format(); }
    std::string format() const;
    friend std::ostream& operator << (std::ostream&, const Json&);
    friend std::istream& operator >> (std::istream&, Json&);
    //
    // compare
    bool operator == (const Json&) const;
    bool operator != (const Json& that) const { return !(*this == that); }
    //
#ifdef WITH_SCHEMA
    // schema
    bool to_schema(std::string* reason);
    bool valid(Json& schema, std::string* reason = nullptr);
#endif
    //
    static Json null, undefined;
    static Json parse(const std::string&);
    static Json array() { return new Array(); }    // returns empty array
    static Json object() { return new Object(); }  // returns empty object
    static int indent;  // for pretty printing
    //
    struct parse_error : std::runtime_error {
        unsigned line = 0, col = 0;
        parse_error(const char* msg, std::istream& in);
    };
    struct use_error : std::logic_error {
        use_error(const char* msg) : std::logic_error(msg) {}
        use_error(const std::string msg) : std::logic_error(msg.c_str()) {}
    };
#ifdef TEST
    static void test() { Node::test(); }
#endif
};

#if defined(_MSC_VER)
	#pragma warning( pop )
#endif

#endif /* JSON11_H_ */
