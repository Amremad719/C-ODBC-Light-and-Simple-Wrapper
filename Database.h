// @author: Amremad719

/*
    A very simple and light wrapper made for making the communication 
    with the ODBC SQL Server Driver much easier

    Designed to work with the .NET Framework v4.7.2
    your luck may vary with older versions
*/

#pragma once
#include <string>
#include <map>
#include <vector>
#include <sqlext.h> 
#include <sql.h>
using namespace std;

#define SQL_RESULT_LEN 240
#define SQL_RETURN_CODE_LEN 1000

struct SQLRET {
public:
    SQLWCHAR Value[SQL_RESULT_LEN];
};

public ref class Database
{
private:

    SQLHANDLE* sqlEnvHandle;
    SQLHANDLE* sqlConnHandle;
    SQLHANDLE* sqlStmtHandle;

    SQLRETURN handleAllocationResult;

    void DAllocHandles()
    {
        //Free all used handles
        SQLFreeHandle(SQL_HANDLE_STMT, sqlStmtHandle);
        SQLFreeHandle(SQL_HANDLE_DBC, sqlConnHandle);
        SQLFreeHandle(SQL_HANDLE_ENV, sqlEnvHandle);
        SQLDisconnect(sqlConnHandle);
    }

    SQLRETURN AllocateHandles()
    {
        /*
        Allocates the handles needed to be able to communicate with the server
        */

        //Allocations and error handling
        SQLRETURN sqlEnvHandleRet = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, sqlEnvHandle);
        if (sqlEnvHandleRet != SQL_SUCCESS) return sqlEnvHandleRet;

        SQLRETURN sqlEnvHandleAttrRet = SQLSetEnvAttr(sqlEnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (sqlEnvHandleAttrRet != SQL_SUCCESS) return sqlEnvHandleAttrRet;

        SQLRETURN sqlConnHandleRet = SQLAllocHandle(SQL_HANDLE_DBC, sqlEnvHandle, sqlConnHandle);
        if (sqlConnHandleRet != SQL_SUCCESS) return sqlConnHandleRet;

        return SQL_SUCCESS;
    }

    wstring constructConnString(const wstring& server, const int& port, const wstring& database, const wstring& username, const wstring& password)
    {
        /*
            Takes the server name, port number, database name, username and password
            and constructs the appropriate connection string to be sent to the server.
        */

        wstring connectionString = L"DRIVER={SQL Server};SERVER=" + server + L", " + to_wstring(port) + L";DATABASE=" + database + L";UID=" + username + L";PWD=" + password + L";";
        return connectionString;
    }

public:

    Database()
    {
        handleAllocationResult = AllocateHandles();
    }

    ~Database()
    {
        DAllocHandles();
    }

    SQLRETURN ConnectToServer(const wstring& server, const int& port, const wstring& database, const wstring& username, const wstring& password)
    {
        /*
            Takes the server name, port number, database name, username and password
            and tries to connect to the server using the given data.
        */

        //connection
        SQLWCHAR retconstring[SQL_RETURN_CODE_LEN];

        //construct the string
        wstring connString = constructConnString(server, port, database, username, password).c_str();

        return SQLDriverConnectW(sqlConnHandle, NULL, (SQLWCHAR*)connString.c_str(), SQL_NTS, retconstring, 1024, NULL, SQL_DRIVER_NOPROMPT);
    }

    pair<SQLRETURN, map<wstring, vector<wstring>>> execStmt(const wstring& stmt)
    {
        /*
            Takes an SQL Server statement, excecutes it and returns the data in this form:

            The first in the pair is an SQLRETURN depicting the successfullness of executing the statement

            The key of the map is the name of the column and the value is a vector of the values in that column
        */

        map<wstring, vector<wstring>> Data;
        SQLRET stmtReturn;
        SQLLEN* pointerStmtValue;

        //Allocate Statement handle
        SQLRETURN StmtHandleRet = SQLAllocHandle(SQL_HANDLE_STMT, sqlConnHandle, sqlStmtHandle);
        if (StmtHandleRet != SQL_SUCCESS) return { StmtHandleRet, Data };

        //Excecute statement
        SQLRETURN stmtExcecRet = SQLExecDirectW(sqlStmtHandle, (SQLWCHAR*)stmt.c_str(), SQL_NTS);
        if (stmtExcecRet != SQL_SUCCESS) return { stmtExcecRet, Data };

        //Fetch Statement return
        memset(stmtReturn.Value, 0, sizeof(stmtReturn.Value));

        SQLSMALLINT* columns = new SQLSMALLINT;
        SQLNumResultCols(sqlStmtHandle, columns);

        while (SQLFetch(sqlStmtHandle) == SQL_SUCCESS)
        {
            for (int i = 0; i < *columns; i++)
            {
                SQLGetData(sqlStmtHandle, i + 1, SQL_WCHAR, &stmtReturn.Value, SQL_RESULT_LEN, pointerStmtValue);

                //Get column name
                SQLWCHAR name[SQL_RESULT_LEN];
                SQLDescribeCol(sqlStmtHandle, i + 1, name, SQL_RESULT_LEN * 2, NULL, NULL, NULL, NULL, NULL);

                Data[wstring(name)].push_back(stmtReturn.Value);
            }
        }
        return { SQL_SUCCESS, Data };
    }
};