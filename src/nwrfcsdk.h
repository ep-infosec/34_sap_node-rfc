// SPDX-FileCopyrightText: 2014 SAP SE Srdjan Boskovic <srdjan.boskovic@sap.com>
//
// SPDX-License-Identifier: Apache-2.0

#ifndef NodeRfc_SDK_H_
#define NodeRfc_SDK_H_

#include "noderfc.h"
#include <sstream>

using namespace Napi;

#define ERRMSG_LENGTH 255
#define ERROR_PATH_NAME_LEN 48
namespace node_rfc
{
    extern Napi::Env __env;

    Napi::Value wrapString(SAP_UC *uc, int length = -1);
    // Napi::Value wrapString(SAP_UC const *uc, int length = -1);

    //
    // Client connection parameters internal representation
    //
    typedef struct _ConnectionParamsStruct
    {
        uint_t paramSize = 0;
        RFC_CONNECTION_PARAMETER *connectionParams = NULL;
        //_ConnectionParamsStruct(uint_t paramSize, RFC_CONNECTION_PARAMETER *connectionParams)
        //    : paramSize(paramSize), connectionParams(connectionParams)
        //{
        //    DEBUG("ConnectionParamsStruct %u", paramSize);
        //}

        ~_ConnectionParamsStruct()
        {
            // DEBUG("~ConnectionParamsStruct ", paramSize);
            if (connectionParams != NULL)
            {
                for (uint_t i = 0; i < this->paramSize; i++)
                {
                    free((void *)connectionParams[i].name);
                    free((void *)connectionParams[i].value);
                }
                free(connectionParams);
                connectionParams = NULL;
            }
        }
    } ConnectionParamsStruct;

    //
    // Client options internal representation
    //
    typedef struct _ClientOptionsStruct
    {
        uint_t bcd = CLIENT_OPTION_BCD_STRING;
        bool stateless = false;
        uint_t timeout = 0;
        RFC_DIRECTION filter_param_type = (RFC_DIRECTION)0;
        Napi::FunctionReference bcdFunction;
        Napi::FunctionReference dateToABAP;
        Napi::FunctionReference dateFromABAP;
        Napi::FunctionReference timeToABAP;
        Napi::FunctionReference timeFromABAP;

        Napi::Value _Value(Napi::Env env)
        {
            Napi::Object options = Napi::Object::New(env);

            // timeout
            options.Set(CLIENT_OPTION_KEY_TIMEOUT, Napi::Number::New(env, timeout));

            // stateless
            options.Set(CLIENT_OPTION_KEY_STATELESS, Napi::Boolean::New(env, stateless));

            // filter
            options.Set(CLIENT_OPTION_KEY_FILTER, Napi::Number::New(env, filter_param_type));

            // bcd
            if (bcd == CLIENT_OPTION_BCD_STRING)
            {
                options.Set(CLIENT_OPTION_KEY_BCD, "string");
            }
            else if (bcd == CLIENT_OPTION_BCD_NUMBER)
            {
                options.Set(CLIENT_OPTION_KEY_BCD, "number");
            }
            else if (bcd == CLIENT_OPTION_BCD_FUNCTION)
            {
                options.Set(CLIENT_OPTION_KEY_BCD, bcdFunction.Value());
            }
            else
            {
                options.Set(CLIENT_OPTION_KEY_BCD, "?");
            }

            // date
            Napi::Object odate = Napi::Object::New(env);
            if (!dateToABAP.IsEmpty())
            {
                odate.Set("toABAP", dateToABAP.Value());
            }
            else
            {
                odate.Set("toABAP", "string");
            }
            if (!dateFromABAP.IsEmpty())
            {
                odate.Set("fromABAP", dateFromABAP.Value());
            }
            else
            {
                odate.Set("fromABAP", "string");
            }
            options.Set(CLIENT_OPTION_KEY_DATE, odate);

            // time
            Napi::Object otime = Napi::Object::New(env);
            if (!timeToABAP.IsEmpty())
            {
                otime.Set("toABAP", timeToABAP.Value());
            }
            else
            {
                otime.Set("toABAP", "string");
            }
            if (!timeFromABAP.IsEmpty())
            {
                otime.Set("fromABAP", timeFromABAP.Value());
            }
            else
            {
                otime.Set("fromABAP", "string");
            }
            options.Set(CLIENT_OPTION_KEY_TIME, otime);

            Napi::EscapableHandleScope scope(env);
            return scope.Escape(options);
        }

        _ClientOptionsStruct &operator=(_ClientOptionsStruct &pool_client_options) // note: passed by copy
        {
            bcd = pool_client_options.bcd;
            stateless = pool_client_options.stateless;
            filter_param_type = pool_client_options.filter_param_type;
            timeout = pool_client_options.timeout;
            // bcd
            if (!pool_client_options.bcdFunction)
            {
                bcdFunction = Napi::Persistent(pool_client_options.bcdFunction.Value());
            }
            // date
            if (!pool_client_options.dateToABAP.IsEmpty())
            {
                dateToABAP = Napi::Persistent(pool_client_options.dateToABAP.Value());
            }
            if (!pool_client_options.dateFromABAP.IsEmpty())
            {
                dateFromABAP = Napi::Persistent(pool_client_options.dateFromABAP.Value());
            }
            // time
            if (!pool_client_options.timeToABAP.IsEmpty())
            {
                timeToABAP = Napi::Persistent(pool_client_options.timeToABAP.Value());
            }
            if (!pool_client_options.timeFromABAP.IsEmpty())
            {
                timeFromABAP = Napi::Persistent(pool_client_options.timeFromABAP.Value());
            }
            return *this;
        };

        ~_ClientOptionsStruct()
        {
            // DEBUG("~ClientOptionsStruct");
            //  bcd
            if (!bcdFunction.IsEmpty())
            {
                bcdFunction.Unref();
            }
            // date
            if (!dateToABAP.IsEmpty())
            {
                dateToABAP.Unref();
            }
            if (!dateFromABAP.IsEmpty())
            {
                dateFromABAP.Unref();
            }
            // time
            if (!timeToABAP.IsEmpty())
            {
                timeToABAP.Unref();
            }
            if (!timeFromABAP.IsEmpty())
            {
                timeFromABAP.Unref();
            }
        }
    } ClientOptionsStruct;

    typedef struct _RfmErrorPath
    {
        RFC_ABAP_NAME functionName;
        RFC_ABAP_NAME parameterName;
        RFC_ABAP_NAME tableName;
        int64_t table_line = -1;
        RFC_ABAP_NAME structureName;
        RFC_ABAP_NAME fieldName;

        void clear()
        {
            functionName[0] = 0;
            parameterName[0] = 0;
            resetPath();
        }

        void resetPath()
        {
            table_line = -1;
            tableName[0] = 0;
            structureName[0] = 0;
            fieldName[0] = 0;
        }

        void setName(RFCTYPE typ, SAP_UC *cName)
        {
            if (typ == RFCTYPE_STRUCTURE)
            {
                strcpyU(structureName, cName);
            }
            else if (typ == RFCTYPE_TABLE)
            {
                strcpyU(tableName, cName);
            }
            else
            {
                strcpyU(fieldName, cName);
            }
        }

        void setFunctionName(SAP_UC *funcName)
        {
            clear();
            strcpyU(functionName, funcName);
        }

        void setParameterName(SAP_UC *pName)
        {
            resetPath();
            strcpyU(parameterName, pName);
        }

        void setFieldName(SAP_UC *fName)
        {
            strcpyU(fieldName, fName);
        }

        Napi::Object getpath()
        {
            Napi::Object path = Napi::Object::New(node_rfc::__env);
            path.Set("rfm", wrapString(functionName));
            path.Set("parameter", wrapString(parameterName));
            if (*tableName)
            {
                path.Set("table", wrapString(tableName));
                path.Set("table_line", table_line);
            }
            if (*structureName)
            {
                path.Set("structure", wrapString(structureName));
            }
            if (*fieldName)
            {
                path.Set("field", wrapString(fieldName));
            }
            return path;
        }

        std::string pathstr()
        {
            std::ostringstream ss;
            Napi::Object obj = this->getpath();
            Napi::Array objKeys = obj.GetPropertyNames();
            ss << "rfmPath: {\n";
            for (uint_t ii = 0; ii < objKeys.Length(); ii++)
            {
                std::string key = objKeys.Get(ii).As<Napi::String>().Utf8Value();
                std::string val = obj.Get(key).ToString().Utf8Value();
                ss << "  " << key << ": " << val << ", \n";
            }
            ss << "}\n";
            return ss.str();
        }

    } RfmErrorPath;

    typedef std::pair<Napi::Value, Napi::Value> ValuePair;
    typedef std::pair<RFC_ERROR_INFO, std::string> ErrorPair;

    // Write parameters (to SDK)
    SAP_UC *setString(const Napi::String napistr);
    SAP_UC *setString(std::string str);
    Napi::Value setRfmParameter(RFC_FUNCTION_DESC_HANDLE functionDescHandle, RFC_FUNCTION_HANDLE functionHandle, Napi::String name, Napi::Value value, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);
    Napi::Value setStructure(RFC_STRUCTURE_HANDLE structHandle, RFC_TYPE_DESC_HANDLE functionDescHandle, SAP_UC *cName, Napi::Value value, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);
    Napi::Value setVariable(RFCTYPE typ, RFC_FUNCTION_HANDLE functionHandle, SAP_UC *cName, Napi::Value value, RFC_TYPE_DESC_HANDLE functionDescHandle, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);

    // Read parameters (from SDK)
    ValuePair getStructure(RFC_TYPE_DESC_HANDLE typeDesc, RFC_STRUCTURE_HANDLE structHandle, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);
    ValuePair getVariable(RFCTYPE typ, RFC_FUNCTION_HANDLE functionHandle, SAP_UC *cName, uint_t cLen, RFC_TYPE_DESC_HANDLE typeDesc, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);
    ValuePair getRfmParameters(RFC_FUNCTION_DESC_HANDLE functionDescHandle, RFC_FUNCTION_HANDLE functionHandle, RfmErrorPath *errorPath, ClientOptionsStruct *client_options);

    // RFC ERRORS
    Napi::Object RfcLibError(RFC_ERROR_INFO *errorInfo);
    Napi::Object AbapError(RFC_ERROR_INFO *errorInfo);
    Napi::Value rfcSdkError(RFC_ERROR_INFO *errorInfo, RfmErrorPath *errorPath = NULL);
    Napi::Value nodeRfcError(std::string message, RfmErrorPath *errorPath = NULL);

    // Connection parameters and client options parsers
    void getConnectionParams(Napi::Object clientParamsObject, ConnectionParamsStruct *clientParams);
    void checkClientOptions(Napi::Object clientOptionsObject, ClientOptionsStruct *clientOptions);

} // namespace node_rfc
#endif
