#include "stdafx.h"
#include "GraoImpl.h"
#include "Objbase.h"
#include "windows.h"
#include <string.h>
#include "stdlib.h"
#include "stdio.h"

//global pointers to the CLASS IDS of Person Object
static CLSID clsidIPersonInfo, clsidPersonInfo; 

//variable that says if the above  clsidIPersonInfo & clsidIPersonInfo are initialized!!!
static bool idsInitialized;  

//json syntax strings!!!
static std::wstring nL(L"\n");  //new line
static std::wstring quote(L"\"");  //double quote 
static std::wstring colon(L":"); //colon
static std::wstring comma(L",");  //comma
static std::wstring shefer(L"|");
static std::wstring tab(L"	");  //tab
static std::wstring bjson(L"{");  //begin of json object
static std::wstring ejson(L"}");   //end  of json object
static std::wstring xml(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
static std::wstring xmlPersonBegin(L"<personinfo>");
static std::wstring xmlPersonEnd(L"</personinfo>");

//a conversion map from win1251 to UNICODE character!!!
//Keys are 1251 symbols & values are their UNICODE equivalents!!!
static std::map<wchar_t, wchar_t> win1251_To_UnicodeMap;

static int bConsoleOpened = 0;
static int bWin1251TounicodeMapinitialized = 0;
static jlong dflags = 0;



/***
Prints flags in console
*/
static void printFlags(jlong flags){
	 int debug = testDFlag(DEBUG_FLAG);
	 if(debug){
		 printf("Flags in getPersonInfo are %d\n", flags);
		 printf("IS_SINGLE_LINE %d\n", testFlag(flags, SINGLE_LINE_FLAG));
		 printf("IS_JSON %d\n", testFlag(flags, JSON_FLAG) );
		 printf("IS_XML %d\n", testFlag(flags, XML_FLAG));
		 printf("IS_SKIP_NULL %d\n", testFlag(flags, SKIP_NULL_FLAG));
		 printf("IS_TSV %d\n", testFlag(flags, TSV_FLAG));
		 printf("IS_CSV %d\n", testFlag(flags, CSV_FLAG));
		 printf("IS_CALC_HASH %d\n", testFlag(flags, CALC_HASH_FLAG));
		 printf("IS_SHEFER %d\n", testFlag(flags, SHEFER));
	 }
}


jstring getPersonInfo(JNIEnv *env,  jstring egn, jlong flags,  std::wstring(*strFun)(PROPERTYNAME_VALUE *, jlong) ){
	 //unsigned short *egnConverted = (unsigned short*)env->GetStringUTFChars(egn, 0);
	 
	 HRESULT hr;
	 HASHES hash;
	 hash.hcSt = NULL;
	 hash.hpSt = NULL;
	 hash.hSt = NULL;

	 long hashes[3];

	 //prints flags in console if debug flag is raised!!!
	 printFlags(flags);

	 //convert EGN to wide char
	 wchar_t* wsz = JavaToWSZ(env, egn);
	 std::wstring json(L"");

	 PROPERTYNAME_VALUE propNameValue[PROP_CNT];
	 //get person info 
	 hr = getPersonInfo(wsz, propNameValue);
	 
	 //set hresult field
	 PROPERTYNAME_VALUE *errCodeProp =  &propNameValue[0];
	 errCodeProp->propValue = numberToString(hr, 16);

	 PWSTR errDesc= NULL;
	 //set error description if errorcode != S_OK
	 if((hr != S_OK) && !testFlag(flags, SKIP_ERR_DESCRIPTION_FLAG)){
		 getSystemErrorDescription(&errDesc, hr);
		 (&propNameValue[1])->propValue = errDesc;
	 }

	  //calculate & print address hashes
	 if(testFlag(flags, CALC_HASH_FLAG)){
		 calcHashes(propNameValue, hashes);
		 fillHashes(&hash, hashes);
		 propNameValue[2].propValue = hash.hpSt;
		 propNameValue[3].propValue = hash.hcSt;
		 propNameValue[4].propValue = hash.hSt;
	 }

	 //create  string from person properties
	 json = strFun(propNameValue, flags);

	 //print result
	 debug_wprintf(L"GetPersonInfo called with EGN %s\n", wsz);
     debug_wprintf(L"Result is\n %s\n", json.c_str());

	
	 //free the allocated string for EGN
	 free(wsz); 
	 
	 // const wchar_t* str = L"Here is a quite long and useless string. Note that this even works with non-ASCII characters, like";  
     //size_t len = wcslen(str); 
	 jstring retval = env->NewString((const jchar*)json.c_str(), json.length());

	 //free err code if not null
	 BSTR errCode= propNameValue[0].propValue;
	 if(errDesc!=NULL) LocalFree(errDesc);
	 if(errCode != NULL) free(errCode);
	 freeHashes(&hash);
	 return retval;  
}


JNIEXPORT jstring JNICALL Java_grao_integration_GraoImpl_getPersonInfo
	(JNIEnv *env, CLASS_OBJECT ob, jstring egn, jlong flagss ){
	return getPersonInfo(env, egn, flagss, createOutPut);
}

//initialize COM
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_initializeCom
(JNIEnv *env, CLASS_OBJECT ob){
	HRESULT res = CoInitialize(NULL);
	if(testDFlag(DEBUG_FLAG))
	printf("CoInitialize called result =  %d... " , res);
}

//close COM
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_unInitializeCom
(JNIEnv *env, CLASS_OBJECT ob){
	CoUninitialize();
	if(testDFlag(DEBUG_FLAG))
	printf("CoUnitialize called... ");
}

//shows console for current process!!!
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_showConsole(JNIEnv *env, CLASS_OBJECT ob){
	showConsole();
}

/**
Sets debug flags in grao dll library from java!!!
*/
JNIEXPORT void JNICALL Java_grao_integration_GraoImpl_setFlags
	(JNIEnv *env, jobject ob, jlong jflags){
		dflags = jflags;
}


/***
  Export used for test in the executable that will load the library!!!!
*/
JNIEXPORT void JNICALL printPersonInfo(WCHAR *idn){
	PROPERTYNAME_VALUE propNameValue [PROP_CNT];
	HRESULT hr  = getPersonInfo(idn,propNameValue);
	//set hresult field
	PROPERTYNAME_VALUE *errCodeProp =  &propNameValue[0];
	wchar_t * str = numberToString(hr, 16);
	errCodeProp->propValue = str;

	PWSTR errDesc= NULL;
	 //set error description if errorcode != S_OK
	 if(!(hr == S_OK)){
		 getSystemErrorDescription(&errDesc, hr);
		 (&propNameValue[1])->propValue = errDesc;
	 }

	std::wstring json = createJson(propNameValue,0);
	free(str);
	
    wprintf(L"printPersonInfo debugFunction export resut is \n %s\n", json.c_str());
	if(errDesc!=NULL) LocalFree(errDesc);
}

//===================================== END OF EXPORTS DEFINITIONS ============================================



/***

Calculates hashes & fills array of 3 items of long
*/
void calcHashes(PROPERTYNAME_VALUE *arraypNamValue, long * outArray){
	long h = 0;
	
	h= calcHash(arraypNamValue[OFFSET_TO_DATA].propValue);
	for(int i = OFFSET_TO_DATA+1; i < 9; i++){
		h^=calcHash(arraypNamValue[i].propValue);
	}
	int debug = testDFlag(DEBUG_FLAG);
	
	ADDRESS adr;
	//permanent address hash
	fillAddress(&adr, arraypNamValue, 9);
	outArray[0] = calcHash(&adr);
	h^=outArray[0];
	if(debug){
		printAddress(&adr);
	}
		 
	//current address hash
	fillAddress(&adr, arraypNamValue, 17);
	outArray[1] = calcHash(&adr);
	h^=outArray[1];
	if(debug){
		printAddress(&adr);
	}

	for(int i = 18; i < PROP_CNT; i++){
		h^=calcHash(arraypNamValue[i].propValue);
	}
	outArray[2] = h;
	return;
}

/**
frees memory for allocated strings
*/
void freeHashes(HASHES *h){
	if(h == NULL) return;
	if(h->hcSt != NULL) free(h->hcSt);
	if(h->hpSt != NULL) free(h->hpSt);
	if(h->hSt != NULL)  free(h->hSt);
}

/***
fills HASHES struct hst from 3 items array of lon with paddress hash, caddress hash & personhash.
*/
void fillHashes(HASHES * hst , long *hl){
	if(hst == NULL || hl ==NULL) return;
	hst->hpSt = numberToString(hl[0], 10);
	hst->hcSt = numberToString(hl[1],10);
	hst->hSt = numberToString(hl[2], 10);
}

/***
Shows console for the current process if console is not shown
*/
static void showConsole(){
	if(!bConsoleOpened){
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		printf("Console opened for calling process...\n");
		bConsoleOpened = 1;
	}
}

/***
Tests   if debug flag is non zero!!!
*/
static jlong testDFlag(jlong flag){
	return (dflags & flag);
}

/***
Tests   if non debug flag is non zero!!!
*/
static jlong testFlag(jlong flags, jlong flag){
	return (flags & flag);
}

/*** 
Table that contains the names of the properties for loaded Person!!!
*/
PROPERTY_NAME  PROPERTY_TABLE [PROP_CNT] = {
	    PROPERTY_NAME(L"ERROR_CODE",L"<ERROR_CODE>", L"</ERROR_CODE>", "ERROR_CODE"),										//0
		PROPERTY_NAME(L"ErrorDescription",L"<ErrorDescription>", L"</ErrorDescription>", "ErrorDescription"),				//1
		PROPERTY_NAME(L"PAddrHash",L"<PAddrHash>", L"</PAddrHash>", "PAddrHash"),											//2
		PROPERTY_NAME(L"CAddrHash",L"<CAddrHash>", L"</CAddrHash>", "CAddrHash"),											//3
		PROPERTY_NAME(L"PersonHash",L"<PersonHash>", L"</PersonHash>", "PersonHash"),										//4
		PROPERTY_NAME(L"Egn",L"<Egn>", L"</Egn>", "Egn" ),																	//5
		PROPERTY_NAME(L"FirstName",L"<FirstName>", L"</FirstName>", "FirstName"),											//6
		PROPERTY_NAME(L"MiddleName",L"<MiddleName>", L"</MiddleName>","MiddleName"),										//7
		PROPERTY_NAME(L"LastName",L"<LastName>", L"</LastName>","LastName"),												//8
		PROPERTY_NAME(L"PAddrDistrict",L"<PAddrDistrict>", L"</PAddrDistrict>","PAddrDistrict"),							//9
		PROPERTY_NAME(L"PAddrMunicipality",L"<PAddrMunicipality>", L"</PAddrMunicipality>","PAddrMunicipality"),			//10
		PROPERTY_NAME(L"PAddrPopulatedPlace",L"<PAddrPopulatedPlace>", L"</PAddrPopulatedPlace>","PAddrPopulatedPlace"),	//11
		PROPERTY_NAME(L"PAddrStreet",L"<PAddrStreet>", L"</PAddrStreet>","PAddrStreet"),									//12
		PROPERTY_NAME(L"PAddrNumber",L"<PAddrNumber>", L"</PAddrNumber>","PAddrNumber"),									//13
		PROPERTY_NAME(L"PAddrEntrance",L"<PAddrEntrance>", L"</PAddrEntrance>","PAddrEntrance"),							//14
		PROPERTY_NAME(L"PAddrFloor",L"<PAddrFloor>", L"</PAddrFloor>","PAddrFloor"),										//15
		PROPERTY_NAME(L"PAddrAppartment",L"<PAddrAppartment>", L"</PAddrAppartment>","PAddrAppartment"),					//16
		PROPERTY_NAME(L"CAddrDistrict",L"<CAddrDistrict>", L"</CAddrDistrict>","CAddrDistrict"),							//17
		PROPERTY_NAME(L"CAddrMunicipality",L"<CAddrMunicipality>", L"</CAddrMunicipality>","CAddrMunicipality"),			//18
		PROPERTY_NAME(L"CAddrPopulatedPlace",L"<CAddrPopulatedPlace>", L"</CAddrPopulatedPlace>","CAddrPopulatedPlace"),	//19
		PROPERTY_NAME(L"CAddrStreet",L"<CAddrStreet>", L"</CAddrStreet>","CAddrStreet"),									//20
		PROPERTY_NAME(L"CAddrNumber",L"<CAddrNumber>", L"</CAddrNumber>","CAddrNumber"),									//21
		PROPERTY_NAME(L"CAddrEntrance",L"<CAddrEntrance>", L"</CAddrEntrance>","CAddrEntrance"),							//22
		PROPERTY_NAME(L"CAddrFloor",L"<CAddrFloor>", L"</CAddrFloor>","CAddrFloor"),										//23
		PROPERTY_NAME(L"CAddrAppartment",L"<CAddrAppartment>", L"</CAddrAppartment>","CAddrAppartment"),					//24
		PROPERTY_NAME(L"Sitizenship",L"<Sitizenship>", L"</Sitizenship>","Sitizenship"),									//25
		PROPERTY_NAME(L"Sitizenship2",L"<Sitizenship2>", L"</Sitizenship2>","Sitizenship2"),								//26
		PROPERTY_NAME(L"IdDocNumber",L"<IdDocNumber>", L"</IdDocNumber>","IdDocNumber"),									//27
		PROPERTY_NAME(L"IdDocDate",L"<IdDocDate>", L"</IdDocDate>","IdDocDate"),											//28
		PROPERTY_NAME(L"IdDocRPU",L"<IdDocRPU>", L"</IdDocRPU>","IdDocRPU"),												//29
		PROPERTY_NAME(L"IdDocRDVR",L"<IdDocRDVR>", L"<IdDocRDVR/>","IdDocRDVR"),											//31
		PROPERTY_NAME(L"IdDocPopulatedPlace",L"<IdDocPopulatedPlace>", L"</IdDocPopulatedPlace>","IdDocPopulatedPlace"),	//32
		PROPERTY_NAME(L"BAddrPopulatedPlace",L"<BAddrPopulatedPlace>", L"</BAddrPopulatedPlace>","BAddrPopulatedPlace"),	//33
		PROPERTY_NAME(L"DeathDate", L"<DeathDate>", L"</DeathDate>","DeathDate")											//34
};

/***
Init filled names in property name value structure and set values to NULL
**/
static void initPropertyNames(PROPERTYNAME_VALUE *arrayp){
	for(int i=0; i < PROP_CNT; i++){
		arrayp[i].propName = PROPERTY_TABLE[i];
		arrayp[i].propValue = NULL;
	}
}


/***
Obtains a pointer to the IPersonInfo Interface
*/
static HRESULT  getIpersonInfo(IPersonInfo **ppPersonInfo){
    HRESULT hr;
	CLSID clsid;
	//IUnknown *pIUnknown;
	//hr = CLSIDFromProgID(L"WinLbd.PersonInfo.1", &clsid);
	hr = IIDFromString(IID_PERSON_INFO_OBJECT, &clsid);
	if(hr != S_OK){
		//wprintf(L"IIDFromString %s returned 0x%x", IID_PERSON_INFO_OBJECT, hr);
		return hr;
	}
	/*OLECHAR *stclsid;
	StringFromCLSID(clsid, &stclsid);
	MULTI_QI mq;
	mq.pIID = &clsid;*/
	
	//CoCreateInstanceEx(clsid, CLSCTX_INPROC_SERVER,NULL,NULL,0,);
	hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)ppPersonInfo);
	//if(hr!=S_OK){ printf("CoCreateInstance returned 0x%x \n", hr); return hr; }
	//hr = pIUnknown->QueryInterface(clsidIPersonInfo, (void **)ppPersonInfo);
	//printf("QueryInterface for clsidIPersonInfo returned 0x%x \n", hr);
	//wprintf(L"CoCreateInstance for  %s returned 0x%x \n", stclsid,  hr);
	return hr;
}



//shows property value in console and in GUI MessageBox!
static void displayProperty(int i, BSTR res){
    int size = wcslen(res);
	if(res!=NULL && wcslen(res) !=0){
		wprintf(PROPERTY_TABLE[i].propNameWideChar);
		 /*
		if(SHOW_GUI)
		MessageBoxW(
			NULL,
			res,
			PROPERTY_TABLE[i].propNameWideChar,
			MB_ICONEXCLAMATION | MB_YESNO
		);*/
		
		wprintf(L" - ");
		wprintf(res);
		wprintf(L"\n");
	}
}

//shows property name & value in console!
static void printProperty(PROPERTYNAME_VALUE *property){
	    if(!testDFlag(DEBUG_FLAG)) return ;
		printf("=======================================================\n");
		if(property->propName.propNameOneByteChar!=NULL) {
			printf("PropertyName = ");
			printf(property->propName.propNameOneByteChar);
		}
		printf("\nValue = "); wprintf(property->propValue == NULL ? L"null" :  property->propValue);
		printf("\n");
		//print character codes!!!
		printStringCharCodes(property->propValue); //printf(" , ");
		printf("\n");
		printf("=======================================================\n");
}

static void printAddress(ADDRESS *pAddress){
	if(pAddress == NULL) return;
	printf("AddrDistrict:");
	wprintf(pAddress->AddrDistrict == NULL ? L"null" : pAddress->AddrDistrict);
	printStringCharCodes(pAddress->AddrDistrict);
	printf("\n");
	printf("AddrMunicipality:");
	wprintf(pAddress->AddrMunicipality == NULL ? L"null" : pAddress->AddrMunicipality);
	printStringCharCodes(pAddress->AddrMunicipality);
	printf("\n");
	printf("AddrPopulatedPlace:");
	wprintf(pAddress->AddrPopulatedPlace == NULL ? L"null" : pAddress->AddrPopulatedPlace);
	printStringCharCodes(pAddress->AddrPopulatedPlace);
	printf("\n");
	printf("AddrStreet:");
	wprintf(pAddress->AddrStreet == NULL ? L"null" : pAddress->AddrStreet);
	printStringCharCodes(pAddress->AddrStreet);
	printf("\n");
	printf("AddrNumber:");
	wprintf(pAddress->AddrNumber == NULL ? L"null" : pAddress->AddrNumber);
	printStringCharCodes(pAddress->AddrNumber);
	printf("\n");
	printf("AddreEntrance:");
	wprintf(pAddress->AddreEntrance == NULL ? L"null" : pAddress->AddreEntrance);
	printStringCharCodes(pAddress->AddreEntrance);
	printf("\n");
	printf("AddrFloor:");
	wprintf(pAddress->AddrFloor == NULL ? L"null" : pAddress->AddrFloor);
	printStringCharCodes(pAddress->AddrFloor);
	printf("\n");
	printf("AddrApartment:");
	wprintf(pAddress->AddrApartment == NULL ? L"null" : pAddress->AddrApartment);
	printStringCharCodes(pAddress->AddrApartment);
	printf("\n");
}

/**
Retrieves person info for EGN in the PROPERTYNAME_VALUE arry!!!
if the function fails all property values are set to NULL!!!
*/
static HRESULT getPersonInfo(BSTR ein, 
							 /*out array of 34 property name values the first is error code PROPERTYNAME_VALUE */
							 PROPERTYNAME_VALUE  *data){
 	HRESULT hr = NULL;
	IPersonInfo *pPersonInfo;

	//init property names with the names from PROPERTY_TABLE & values are set to NULL
	initPropertyNames(data);
	
	//obtain pointer to COM object!
    hr = getIpersonInfo(&pPersonInfo);
	
	if(hr != S_OK) return hr;

	if(pPersonInfo != NULL){
		//load person by ein 
		hr = pPersonInfo->Load(ein);
		//printf("Load EIN %s returned 0x%x\n", ein, hr);
		
		//retrieve data if person loaded successfully!!!
		if(hr == S_OK){
			int propTableLength = sizeof(PROPERTY_TABLE)/sizeof(PROPERTY_TABLE[0]);
			for(int i = OFFSET_TO_DATA; i < propTableLength; i++){
				hr = pPersonInfo->GetValue(PROPERTY_TABLE[i].propNameWideChar, &(data[i].propValue));

				//convert to unicode
				convertWin1251ToUnicode(data[i].propValue);
				
				//print property if flag is set
				printProperty(&data[i]);
			}
			//printf("\n");
		}
		//Free Object finally
		pPersonInfo->Release();
	}
	return hr;
}




/***
Creates output string from properties based on the flags parameter!
*/
static std::wstring createOutPut(PROPERTYNAME_VALUE *arraypNamValue, jlong flags){
	if(testFlag(flags, JSON_FLAG)) return createJson(arraypNamValue, flags);
	if(testFlag(flags, XML_FLAG))  return createXml(arraypNamValue, flags);
	if(testFlag(flags, CSV_FLAG))  return createDelimiterSeparatedString(arraypNamValue, comma);
	if(testFlag(flags, TSV_FLAG))  return createDelimiterSeparatedString(arraypNamValue, tab);
	if(testFlag(flags, SHEFER))    return createDelimiterSeparatedString(arraypNamValue, shefer);
	return createJson(arraypNamValue, flags);
}

/***
Takes only not null properties indexes into a  result array - array must  PROP_CNT integers  in length!!!
*/
static int notNullFilter(PROPERTYNAME_VALUE *arraypNamValue, int *resultArray){
	int i=0;
	int cnt = 0;  //number of not null properties

	for(; i < PROP_CNT; i++){
		BSTR currentPropertyValue = arraypNamValue[i].propValue;
		if(currentPropertyValue == NULL || wcslen(currentPropertyValue) == 0) continue;
		resultArray[cnt++] = i;//&arraypNamValue[i];  //store index in result array!!!
	}
	return cnt;
}

/**
Creates a json string out of property-Name array!
*/
static std::wstring createJson(PROPERTYNAME_VALUE *arraypNameValue, jlong flags){
	std::wstring res(L"");	
	int i = 0;
	int cnt = PROP_CNT;
	jlong isSingleLine = testFlag(flags, SINGLE_LINE_FLAG); //check if new line must be added between properties in result string
	std::wstring lbjson = L"{";
	if(!isSingleLine) lbjson+L"\n";


	//local array of integers storing indexes  only of not null and not empty properties!!!
	int indexarray[PROP_CNT]; for(int j=0; j < PROP_CNT; j++) indexarray[j] = j;
	PROPERTYNAME_VALUE *parray = arraypNameValue;

	if(testFlag(flags, SKIP_NULL_FLAG)){
		cnt = notNullFilter(arraypNameValue, indexarray);
	}
	if(cnt == 0) return res;

	i=0;
	for(; i < cnt-1; i++){
		std::wstring propName(parray[indexarray[i]].propName.propNameWideChar);
		std::wstring propValue(parray[indexarray[i]].propValue == NULL ? L"" : parray[indexarray[i]].propValue);  //this is not supposed to happen but just in case!!!
		if(!isSingleLine) res+=(quote + propName + quote + colon + quote + propValue + quote + comma + nL);
		else res+=(quote + propName + quote + colon + quote + propValue + quote + comma);
	}

	std::wstring propName(parray[indexarray[i]].propName.propNameWideChar);
	std::wstring propValue(parray[indexarray[i]].propValue == NULL ? L"" : parray[indexarray[i]].propValue);
	if(!isSingleLine) res+=(quote + propName + quote + colon + quote + propValue + quote +  nL);
	else res+=(quote + propName + quote + colon + quote + propValue + quote);
	res= lbjson + res + ejson;

	return res;

}


/***
Creates xml sting out of property-Name array!
*/
static std::wstring createXml(PROPERTYNAME_VALUE *arraypNameValue, jlong flags){
	std::wstring res(L"");
	
	int i = 0;
	int cnt = PROP_CNT;
	jlong isSingleLine = testFlag(flags, SINGLE_LINE_FLAG);

	//local array of integers storing indexes  only of not null and not empty properties!!!
	int indexarray[PROP_CNT]; for(int j=0; j < PROP_CNT; j++) indexarray[j] = j;
	PROPERTYNAME_VALUE *parray = arraypNameValue;

	if(testFlag(flags, SKIP_NULL_FLAG)){
		cnt = notNullFilter(arraypNameValue, indexarray);
	}
	if(cnt == 0) return res;

	for(i; i < cnt; i++){
		std::wstring propValue(parray[indexarray[i]].propValue == NULL ? L"" : parray[indexarray[i]].propValue);
		std::wstring xmlB(parray[indexarray[i]].propName.pXmlBegin);
		std::wstring xmlE(parray[indexarray[i]].propName.pXmlEnd);
		if(isSingleLine) res+=(xmlB + propValue +  xmlE);
		else res+=(xmlB + propValue +  xmlE + nL);
	}
	if(isSingleLine)  return xml + xmlPersonBegin + res + xmlPersonEnd;
	return xml + nL + xmlPersonBegin + nL + res + xmlPersonEnd + nL;
}


/**
Converts a java string to wide char string!!!
*/
static wchar_t * JavaToWSZ(JNIEnv* env, jstring string)
{
    if (string == NULL)
        return NULL;
    int len = env->GetStringLength(string);
    const jchar* raw = env->GetStringChars(string, NULL);
    if (raw == NULL)
        return NULL;

    wchar_t* wsz = new wchar_t[len+1];
    memcpy(wsz, raw, len*sizeof(wchar_t));
    wsz[len] = 0;

    env->ReleaseStringChars(string, raw);

    return wsz;
}

/**
Converts number to string! returns a pointer to allocated string !
The caller must free memory!
*/
static wchar_t * numberToString(long l, int radix){
	wchar_t * str = (wchar_t *)malloc(33*sizeof(wchar_t));
	if(str == NULL) return str;
	memset(str, 0, 33*sizeof(wchar_t));  //zero the memory
    _ltow(l, str,  radix);  //convert to hex string
	//wprintf(cc);
	return str;
}

/**
Prints the codes of a wide char string!!!
*/
void printStringCharCodes(wchar_t *st){
	if((!testDFlag(DEBUG_FLAG)) || (st == NULL)) return;
	int len = wcslen(st);
	printf("UNICODE char codes: ");
	for(int i = 0; i < len; i++){
		long l =  st[i];
		printf("0x%x, ", l);
	}
	printf("\nString length = %d", len);
}

/**
Calculates hash code of BSTR string
*/
long calcHash(BSTR str){
	if(str == NULL) return 0;
	int h = 0;
	int len = wcslen(str);
	for(int i=0; i < len; i++){
		 h = 31 * h + str[i];
	}
	return h;
}

/**
Calculates address hashCode
*/
long calcHash(ADDRESS *address){
	if(address == NULL) return 0;
	return calcHash(address->AddrDistrict) ^ 
	calcHash(address->AddrMunicipality) ^ 
	calcHash(address->AddrPopulatedPlace) ^ 
	calcHash(address->AddrStreet) ^ 
	calcHash(address->AddrNumber) ^ 
	calcHash(address->AddreEntrance) ^ 
	calcHash(address->AddrFloor) ^ 
	calcHash(address->AddrApartment);
}

/**
Fills address from PROPERTYNAME_VALUE array & offset that show the start inde of address properties in PROPERTYNAME_VALUE array!
*/
void fillAddress(ADDRESS *address, PROPERTYNAME_VALUE  *data, int offset){
	if(address == NULL) return;
	int i = 0;
	address->AddrDistrict = data[offset+(i++)].propValue;
	address->AddrMunicipality = data[offset+(i++)].propValue;
	address->AddrPopulatedPlace = data[offset+(i++)].propValue;
	address->AddrStreet =  data[offset+(i++)].propValue;
	address->AddrNumber =  data[offset+(i++)].propValue;
	address->AddreEntrance =  data[offset+(i++)].propValue;
	address->AddrFloor =  data[offset+(i++)].propValue;
	address->AddrApartment =  data[offset+(i++)].propValue;
}



/***
Repalces win1251 cyrillic symbols with UNICODE Equivalents from win1251_To_UnicodeMap!
*/
static void convertWin1251ToUnicode(wchar_t * st){
	if(st == NULL) return;
	 
	std::map<wchar_t, wchar_t>::iterator it;

	//replace
	int len = wcslen(st);
	for(int i = 0; i < len; i++){
		//get char from map
		wchar_t currentSymbol =  st[i];
		it = win1251_To_UnicodeMap.find(currentSymbol);

		//if char is present in map replace it with unicode symbol!!!
		if (it != win1251_To_UnicodeMap.end()){
			st[i] = it->second;
		}
	}
}



/**
The same as printf but only allowed if DEBUG_FLAG is raised!!!
*/
int __cdecl debug_printf(const char * format, ...){
	   if(!testDFlag(DEBUG_FLAG)) return 0;
	    va_list args;
		va_start(args, format);
		printf(format, args);
		va_end(args);
		return 0;
}

/*
The same as wprintf but only allowed if DEBUG_FLAG is raised!!!
*/
int __cdecl debug_wprintf(const wchar_t * format, ...){
	 if(!testDFlag(DEBUG_FLAG)) return 0;
	 va_list args;
	 va_start(args, format);
	 wprintf(format, args);
	 va_end(args);
	 return 0;
}



/***
Creates delimiter separated  sting out of property-Name array! Single line flag is not checked! Always returns single line string!!
Not  null properties flag is also ignored!!
*/
static std::wstring createDelimiterSeparatedString(PROPERTYNAME_VALUE *arraypNameValue, std::wstring delimiter){
	std::wstring res(L"");
	int i = 0;
	int cnt = PROP_CNT;
	
	for(i; i < cnt-1; i++){
		std::wstring propValue(arraypNameValue[i].propValue == NULL ? L"" : arraypNameValue[i].propValue);
		res+=(propValue + delimiter);
	}
	std::wstring propValue(arraypNameValue[i].propValue == NULL ? L"" : arraypNameValue[i].propValue);
	res+=(propValue);
	
	return res;
}


/**
Returns a description of system error for the specified system error number!!!
The caller must free memory when string message is not needed any more by calling LocalFree()!!!
*/
DWORD  getSystemErrorDescription(PWSTR *resultString, DWORD errNumber){
    return FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errNumber, 0, (PWSTR)resultString, 0, NULL);

}


/**
Initializes win1251 to Unicode conversion map!!!
*/
void initWin1251TounicodeMap(){
	if(bWin1251TounicodeMapinitialized) return;
	
	win1251_To_UnicodeMap.clear();
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x80, 0x402));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x81, 0x403));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x82, 0x201a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x83, 0x0453));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x84, 0x201e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x85, 0x2026));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x86, 0x2020));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x87, 0x2021));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x88, 0xfffe));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x89, 0x2030));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8a, 0x0409));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8b, 0x2039));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8c, 0x040a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8d, 0x040c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8e, 0x040b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x8f, 0x040f));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x90, 0x0452));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x91, 0x2018));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x92, 0x2019));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x93, 0x201c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x94, 0x201d));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x95, 0x2022));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x96, 0x2013));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x97, 0x2014));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x98, 0xfffe));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x99, 0x2122));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9a, 0x0459));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9b, 0x203a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9c, 0x045a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9d, 0x045c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9e, 0x045b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0x9f, 0x045f));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa0, 0x00a0));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa1, 0x040e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa2, 0x045e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa3, 0x0408));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa4, 0x00a4));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa5, 0x0490));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa6, 0x00a6));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa7, 0x00a7));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa8, 0x0401));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xa9, 0x00a9));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xaa, 0x0404));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xab, 0x00ab));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xac, 0x00ac));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xad, 0x00ad));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xae, 0x00ae));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xaf, 0x0407));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb0, 0x00b0));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb1, 0x00b1));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb2, 0x0406));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb3, 0x0456));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb4, 0x491));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb5, 0x00b5));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb6, 0x00b6));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb7, 0x00b7));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb8, 0x0451));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xb9, 0x2116));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xba, 0x0454));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xbb, 0x00bb));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xbc, 0x0458));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xbd, 0x0405));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xbe, 0x0455));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xbf, 0x0457));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc0, 0x0410));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc1, 0x0411));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc2, 0x0412));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc3, 0x0413));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc4, 0x0414));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc5, 0x0415));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc6, 0x0416));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc7, 0x0417));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc8, 0x418));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xc9, 0x0419));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xca, 0x041a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xcb, 0x041b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xcc, 0x041c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xcd, 0x041d));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xce, 0x041e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xcf, 0x041f));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd0, 0x0420));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd1, 0x0421));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd2, 0x0422));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd3, 0x0423));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd4, 0x0424));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd5, 0x0425));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd6, 0x0426));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd7, 0x0427));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd8, 0x0428));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xd9, 0x0429));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xda, 0x042a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xdb, 0x042b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xdc, 0x042c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xdd, 0x042d));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xde, 0x042e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xdf, 0x042f));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe0, 0x430));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe1, 0x431));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe2, 0x432));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe3, 0x433));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe4, 0x434));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe5, 0x435));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe6, 0x436));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe7, 0x437));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe8, 0x438));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xe9, 0x439));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xea, 0x43a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xeb, 0x43b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xec, 0x43c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xed, 0x43d));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xee, 0x43e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xef, 0x43f));

	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf0, 0x440));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf1, 0x441));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf2, 0x442));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf3, 0x443));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf4, 0x444));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf5, 0x445));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf6, 0x446));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf7, 0x447));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf8, 0x448));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xf9, 0x449));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xfa, 0x44a));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xfb, 0x44b));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xfc, 0x44c));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xfd, 0x44d));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xfe, 0x44e));
	win1251_To_UnicodeMap.insert(std::pair<wchar_t, wchar_t>(0xff, 0x44f));

	printf("Win1251 to Unicode map initialized!\n");

	bWin1251TounicodeMapinitialized = 1;
}


/*
static HRESULT  getIpersonInfo(IPersonInfo **ppPersonInfo){
	HRESULT hr;
	IPersonInfo *pInfo = NULL;
	*ppPersonInfo = NULL;
    IUnknown *pIUnknown;
	

	//initialize the class identifiers
	if(!idsInitialized) {
		hr = IIDFromString(IID_PERSON_INFO_OBJECT, &clsidPersonInfo);
	    hr = IIDFromString(IID_IPERSON_INFO, &clsidIPersonInfo); 
		idsInitialized = 1;
		printf("Initializing class IDS\n");
		OLECHAR* guidString;
		StringFromCLSID(clsidPersonInfo, &guidString);
		wprintf(L"clsidPersonInfo = %s\n", guidString);
		CoTaskMemFree(guidString);
		StringFromCLSID(clsidIPersonInfo, &guidString);
		wprintf(L"clsidIPersonInfo = %s \n", guidString);
		CoTaskMemFree(guidString);
	}


	//create COM object instance
	hr = CoCreateInstance(clsidPersonInfo, 0, CLSCTX_INPROC_SERVER , IID_IUnknown, (void **)&pIUnknown);
	//hr = CoCreateInstance(clsidIPersonInfo, 0, CLSCTX_INPROC_SERVER , clsidIPersonInfo, (void **)ppPersonInfo);
	printf("CoCreateInstance for clsidPersonInfo  returned 0x%x \n",  hr);
	if(hr != S_OK){  return hr;}

	//retrieve pointer to IPersonInfo Interface
	hr = pIUnknown->QueryInterface(clsidIPersonInfo, (void **)ppPersonInfo);
	printf("QueryInterface for clsidIPersonInfo returned 0x%x \n", hr);

	if(hr != S_OK){ return hr;}
	pIUnknown->Release();
	return hr;
}*/



