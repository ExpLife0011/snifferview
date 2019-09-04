//sqlite3封装层 2019 01 05
#ifndef SQLITEOPT_COMSTATIC_H_H_
#define SQLITEOPT_COMSTATIC_H_H_
#include <Windows.h>
#include <map>
#include <list>
#include "mstring.h"
#include "sqlite3.h"
#pragma warning(disable:4251)

class SqliteException {
public:
    SqliteException(const std::mstring &err) {
        mErr = err;
    }

    virtual ~SqliteException() {}

    const char *what() {
        return mErr.c_str();
    }

private:
    std::mstring mErr;
};

struct IteratorCache {
    std::map<std::mstring, std::mstring> mCurData;
    IteratorCache *mNext;
};

class SqliteIterator {
    friend class SqliteResult;
public:
    SqliteIterator();
    SqliteIterator(const SqliteIterator &copy);
    SqliteIterator(const IteratorCache *it);
    virtual ~SqliteIterator();
    SqliteIterator &operator=(const SqliteIterator &copy);
    SqliteIterator operator++();
    bool operator==(const SqliteIterator &dst);
    bool operator!=(const SqliteIterator &dst);

public:
    std::mstring GetValue(const std::mstring &name) const;
    SqliteIterator GetNext();

private:
    const IteratorCache *mData;
};

class SqliteResult {
    friend class SqliteOperator;
public:
    SqliteResult();
    void SetResult(const std::list<IteratorCache *> *mResultSet);
    bool IsValid();
    bool IsEmpty();
    SqliteIterator begin();
    SqliteIterator end();

    size_t GetSize() const;
    SqliteIterator GetNode(int);
    SqliteIterator GetFirst();
    SqliteIterator GetLast();

private:
    const std::list<IteratorCache *> *mResultSet;
};

class SqliteOperator {
public:
    SqliteOperator();
    SqliteOperator(const std::mstring &filePath);
    virtual ~SqliteOperator();

    bool Open(const std::mstring &filePath);
    bool IsOpen();
    void Close();
    SqliteResult &Select(const std::mstring &sql);
    bool Update(const std::mstring &sql);
    bool Delete(const std::mstring &sql);
    bool Insert(const std::mstring &sql);
    bool Exec(const std::mstring &sql);
    bool TransBegin();
    bool TransSubmit();
    std::mstring GetError();

    //对输入输出进行编解码,主要做两件事,转义和UTF8编码
    std::mstring EncodeStr(const std::mstring &str);
    std::mstring DecodeStr(const std::mstring &str);
private:
    void Clear();
    static int SelectCallback(void *data, int argc, char **argv, char **name);

private:
    bool mInit;
    sqlite3 *mDb;
    std::mstring mDbPath;
    SqliteResult mResult;
    std::mstring mError;
    //迭代器内存由Operator统一分配和管理，生命周期等同于Operator对象
    //迭代器对象可以随意copy，其中的数据内存由Operator统一调度
    std::list<IteratorCache *> mCacheSet;
};
#endif //SQLITEOPT_COMSTATIC_H_H_