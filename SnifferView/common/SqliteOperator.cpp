#include "SqliteOperator.h"
#include "StrUtil.h"

using namespace std;

SqliteIterator::SqliteIterator() {
    mData = NULL;
}

SqliteIterator::SqliteIterator(const SqliteIterator &copy) {
    mData = copy.mData;
}

SqliteIterator::SqliteIterator(const IteratorCache *it) {
    mData = it;
}

SqliteIterator::~SqliteIterator() {
}

SqliteIterator &SqliteIterator::operator=(const SqliteIterator &copy) {
    mData = copy.mData;
    return *this;
}

SqliteIterator SqliteIterator::operator++() {
    mData = mData->mNext;
    return *this;
}

bool SqliteIterator::operator!=(const SqliteIterator &dst) {
    return (mData != dst.mData);
}

mstring SqliteIterator::GetValue(const mstring &name) const {
    map<mstring, mstring>::const_iterator it = mData->mCurData.find(name);
    if (mData->mCurData.end() != it)
    {
        return it->second;
    }
    return "";
}

SqliteIterator SqliteIterator::GetNext() {
    return mData->mNext;
}

SqliteResult::SqliteResult() {
}

void SqliteResult::SetResult(const std::list<IteratorCache *> *resultSet) {
    mResultSet = resultSet;
}

bool SqliteResult::IsValid() {
    return mResultSet != NULL && !mResultSet->empty();
}

bool SqliteResult::IsEmpty() {
    return !IsValid();
}

SqliteIterator SqliteResult::begin() {
    if (mResultSet == NULL || mResultSet->empty())
    {
        return NULL;
    }

    return *(mResultSet->begin());
}

size_t SqliteResult::GetSize() const {
    if (mResultSet == NULL || mResultSet->empty())
    {
        return 0;
    }

    return mResultSet->size();
}

SqliteIterator SqliteResult::end() {
    return NULL;
}

SqliteOperator::SqliteOperator() {
    mInit = false;
    mDb = NULL;
}

SqliteOperator::SqliteOperator(const std::mstring &filePath) {
    Open(filePath);
}

bool SqliteOperator::IsOpen() {
    return (mDb != NULL);
}

SqliteOperator::~SqliteOperator() {
    Close();
    Clear();
}

void SqliteOperator::Clear() {
    for (list<IteratorCache *>::iterator it = mCacheSet.begin() ; it != mCacheSet.end() ; it++)
    {
        delete (*it);
    }
    mCacheSet.clear();
}

int SqliteOperator::SelectCallback(void *data, int argc, char **argv, char **name) {
    SqliteOperator *ptr = (SqliteOperator *)data;
    IteratorCache *newData = new IteratorCache();
    newData->mNext = NULL;
    for (int i = 0 ; i < argc ; i++)
    {
        newData->mCurData.insert(make_pair(ptr->DecodeStr(name[i]), ptr->DecodeStr(argv[i])));
    }

    if (!ptr->mCacheSet.empty())
    {
        IteratorCache *endNode = *(ptr->mCacheSet.rbegin());
        endNode->mNext = newData;
    }
    ptr->mCacheSet.push_back(newData);
    return 0;
}

bool SqliteOperator::Open(const std::mstring &filePath) {
    mInit = (0 == sqlite3_open(filePath.c_str(), &mDb));

    if (!mInit)
    {
        sqlite3_close(mDb);
        mDb = NULL;
        throw SqliteException(FormatA("打开数据库文件%hs失败", filePath.c_str()));
    }
    return mInit;
}

void SqliteOperator::Close() {
    if (mDb)
    {
        sqlite3_close(mDb);
        mDb = NULL;
    }
}

SqliteResult &SqliteOperator::Select(const std::mstring &sql) {
    char *err = NULL;
    Clear();
    if (SQLITE_OK != sqlite3_exec(mDb, sql.c_str(), SelectCallback, this, &err))
    {
        throw SqliteException(err);
    }
    mResult.SetResult(&mCacheSet);
    return mResult;
}

bool SqliteOperator::Update(const std::mstring &sql) {
    Exec(sql.c_str());
    return true;
}

bool SqliteOperator::Delete(const std::mstring &sql) {
    Exec(sql.c_str());
    return true;
}

bool SqliteOperator::Insert(const std::mstring &sql) {
    Exec(sql.c_str());
    return true;
}

bool SqliteOperator::Exec(const std::mstring &sql) {
    char *err = NULL;
    if (SQLITE_OK != sqlite3_exec(mDb, sql.c_str(), NULL, NULL, &err))
    {
        throw SqliteException(err);
    }

    mError = err;
    return true;
}

mstring SqliteOperator::GetError() {
    return mError;
}

bool SqliteOperator::TransBegin() {
    return true;
}

bool SqliteOperator::TransSubmit() {
    return true;
}

mstring SqliteOperator::EncodeStr(const std::mstring &str) {
    mstring result = str;
    result.repsub("/", "//");
    result.repsub("'", "''");
    result.repsub("[", "/[");
    result.repsub("]", "/]");
    result.repsub("%", "/%");
    result.repsub("&","/&");
    result.repsub("_", "/_");
    result.repsub("(", "/(");
    return AtoU(result);
}

mstring SqliteOperator::DecodeStr(const std::mstring &str) {
    mstring result = UtoA(str);
    result.repsub("//", "/");
    result.repsub("''", "'");
    result.repsub("/[", "[");
    result.repsub("/]", "]");
    result.repsub("/%", "%");
    result.repsub("/&", "&");
    result.repsub("/_", "_");
    result.repsub("/(", "(");
    return result;
}