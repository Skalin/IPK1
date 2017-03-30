# IPK Projekt 1
##### Dominik Skála, xskala11
##### Fakulta Informačních technologií, Vysoké učení technické v Brně

##### klient-server aplikace využívající RESTful API. REST a BSD socketů 



##### Překlad programů je zajištěn za pomocí makefile. 
| make příkazy | Operace |
| :-------------------------- | --------:|
| **make** | překlad klienta i serveru, klient je spustitelný jako **ftrest**, server jako **ftrestd**, před překladem se smažou veškeré předchozí verze programů |
| **make all** | alias pro příkaz **make** |
| **make ftrest** | překlad klienta, spustitelný jako **ftrest** |
| **make ftrestd** | překlad serveru, spustitelný jako **ftrestd** |
| **make clean** | dojde ke smazání všech zastaralých verzí programů |


## Klient = **ftrest**
##### $ **ftrest COMMAND REMOTE-PATH [LOCAL-PATH]**
| Parametr | Popis |
|:-----:| :-----:|
|COMMAND | PUT/GET/DELETE |
|REMOTE-PATH| cesta k **souboru nebo adresáři** na serveru |
|LOCAL PATH | cesta v lokálním souborovém systému, povinné pro akci PUT |

##### **PŘÍKLAD**

##### $ ftrest PUT http://localhost:12345/tonda/foo/bar/doc.pdf ~/doc.pdf
* Nahrání souboru doc.pdf na serveru do adresáře bar


##### $ ftrest DELETE /USER-ACCOUNT/REMOTE-PATH?type=[file|folder] HTTP/1.1
* Smazání souboru/adresáře

### Rozšíření
Bylo implementováno rozšíření autorizace uživatelů. To je řešeno následujícím způsobem:
Uživatel spustí klientskou aplikaci, ta se jej dotáže na heslo, při správném zadání všech parametrů se hlavička zašle na server, ten ji zpracuje otevřením souboru **userpw**, který **MUSÍ** být umístěn v root složce, pokud zde není, server není možné spustit. Samozřejmostí je automatický zákaz stahování všech souborů s názvem **userpw**.
Po přijetí hlavičky server provede kontrolu validity a buď pokračuje zpracováním příkazu, či vrátí chybové hlášení
Během autorizace nedochází k přenosu přes HTTPS, také není účelem projektu poukázat na možnosti a nemožnosti zabezpečení. Z klienta se zasílá Authorization tag: Basic heslo, nedochází však k zakódování do soustavy 64, základní knihovny pro kódování do této soustavy nejsou přítomny na referenčním stroji a formát tohoto kódování je stejně bezpečný jako nekódování samotné. Pokud bychom chtěli tuto komunikaci dostatečně zabezpečit, byli bychom nuceni užít knihovny funkce sha256 pro dostatečně zabezpečení hesla. Pro náš projekt však stačí heslo nezakodované.

## Implementace klienta
* Klient provede kontrolu agrumentů a požádá od uživatele o heslo.
* Vytvoří se základní zpráva obsahující HTTP hlavičku dle argumentů, konec hlavičky je vždy posloupnost znaků "**\r\n\r\n**". V případě, že je použit příkaz **put**, je za konec hlavičky přiložen i obsah odesílaného souboru. Klient však nemůže odesílat zprávy delší než 1024 znaků. To je za účelem snížení náporu klienta na server a tedy zkrácení jeho nutných požadavků.
* Následně se pomocí funkce **gethostbyname()** získá adresa serveru (dočasně uložena ve struktuře **hostent**).
* Vytvořím druhou strukturu typu **sockaddr_in**, do které uložím potřebné položky, také je do ní uložena délka adresy a adresa samotná ze struktury **hostent**.
* Do proměnné typu **int** si uložím výsledek funkce **socket()** a zkontroluji, zda byl vytvořen správně
* Za pomocí funkce **connect()** se připojím na socket a skrze funkci **send()** zašlu dříve vytvořenou zprávu.
* Jakmile server zprávu zpracuje a pošle odpověď, pomocí základního dělení zprávy (opět je hlavička ze serveru oddělena stejným způsobem od zbytku dat jako hlavička zasílaná klientem) získám potřebné informace o tom, jak dopadla operace.
* Pokud je ve výsledku uložen "Success.", operace je úspěšně dokončena a klient se ukončí se stavem 0. Pokud došlo k jakémukoliv jinému stavu, klient o tom obeznámí uživatele tiskem na standardní chybový výstup a ukončí svou práci s chybovým stavem **EXIT_FAILURE**.

## HTTP hlavička požadavku
Tato hlavička musí obsahovat následující položky:

| Označení | Text |
| :------------- | -------------: |
|Host| Adresa hostitele |
|Date | Timestamp klienta v době vytvoření požadavku|
|Accept | Požadavaný typ obsahu pro odpověď |
|Accept-Encoding | Podporový způsob kódování dat (identity, gzip, deflate)|
|Content-Type | MIME typ obsahu požadavku (pro PUT či POST)|
|Content-Length | Délka obsahu požadavku (pro PUT či POST)|
|Authorization | typ a heslo ve formátu **Basic heslo** (neodpovídá úplně specifikaci)|

## Základní příkazy
| Kód      | Text         |
| :------------- |-------------:|
|del | smaže soubor určený REMOTE-PATH na serveru |
|get | zkopíruje soubor z REMOTE-PATH do aktuálního lokálního adresáře či na místo určené pomocí LOCAL-PATH je-li uvedeno|
|put | zkopíruje soubor z LOCAL-PATH do adresáře REMOTE-PATH|
|lst | vypíše obsah vzdáleného adresáře na standardní výstup (formát bude stejný jako výstup z příkazu ls), před výsledkem ls se vypíše ještě info status operace a nový řádek, pro vizuální oddělení výstupu|
|mkd | vytvoří adresář specifikovaný v REMOTE-PATH na serveru|
|rmd | odstraní adresář specifikovaný V REMOTE-PATH ze serveru|


## HTTP Odpovědi
| Kód    | Textová hodnota |    Výsledek |
| :---- | :----------: | -----:|
| 200     | OK | operace byla provedena úspěšně |
| 400 | Bad Request     |    při přístupu k objektu jiného typu než uvedeného v požadavku (požadavek na operaci nad souborem, ale REMOTE-PATH ukazuje na adresář), špatná hlavička, atd. |
| 401 | Unauthorized | uživatel nebyl autorizován, bylo zadáno špatné heslo |
| 404     | Not Found     |   objekt (soubor/adresář) v požadavku neexistuje |

***

## Server **ftrestd**

##### $ **ftrestd [-r ROOT-FOLDER] [-p PORT]**
* **-r ROOT-FOLDER** - specifikuje kořenový adresář, kde budou ukládány soubory pro jednotlivé uživatele, výchozí hodnota je aktuální 
* **-p PORT** - specifikuje port, na kterém bude server naslouchat, implicitně 6677

### Implementace serveru
* Zkontroluji a zpracuji vstupní argumenty (jestli je port ve správném rozsahu atp.,). Není-li port zadán, server naslouchá implicitně na portu **6677**
* Je zkontrolována přítomnost souboru **userpw**, bez něj není nikdy možné spustit server.
* Vytvořím server socket za pomocí fce **socket()**
* Vytvořím strukturu **sockadr_in** pro server. 
* Pomocí fce **bind()** bindnu socket a uložím do proměnné
* Spustím fci **listen()** nad socketem
* Pomocí nekončného cyklu naslouchám na socketu na spojení, pokud dojde ke spojení, přijmu pomocí druhého cyklu komunikaci, je zpracována hlavička funkcí **parseRecievedMessage()**, ověřeno heslo funkcí **checkPassword()**, následně je určena podle funkce **whatToDo()**, která operace se bude vykonávat (rozsah **0 - 5**) a pro každou operaci je poté adekvátně spuštěná funkce.
* Pro každý výsledek operace je vytvořena hlavička, je adekvátně naplněna daty.
* Následně je za hlavičku uložen textový výsledek operace, který klient přepošle na standardní výstup, v případě, že je zasílán další info (výpis ls, obsah souboru), zašle se i toto.
* Za pomocí funkce send je klientovi zaslána odpověď, v případě delších odpovědí je využito cyklů s funkcí **send()**, které pošlou korektně zprávu.

## Http hlavička odpovědi
| Parametr      | Popis parametru         |
| :------------- | -------------:|
|Date | Timestamp serveru v době vyřízení požadavku|
|Content-Type | typ obsahu odpovědi podle MIME|
|Content-Length | délka obsahu odpovědi|
|Content-Encoding | typ kódování obsahu (identity, gzip, deflate)|


V každé zprávě ze serveru je také zasílán adekvátní výsledek operace, která se v klientovi tiskne na standardní výstup. Klient pouze výsledek tiskne, nemusí jej nijak kontrolovat, kontrolu zaručuje server. Určité operace jsou však zasílány vždy pouze a jen s určitým HTTP kódem.
Níže je jejich soupis:
## Výsledky operací
| HTTP | Text | Výsledek |
| :--- | :---: | ------: |
| 200 | Success. | Operace byla úspěšně dokončena |
| 401 | Unauthorized. | Zadané heslo nebylo validní |
| 404 | User Account Not Found. | Zadaný uživatel neexistuje |
| 404 | Directory not found. | REMOTE-PATH neukazuje na žádný existující adresář při použití operace lst, rmd |
| 404 | File not found. | REMOTE-PATH neukazuje na žádný existující objekt při použití operace del, get, případně i put (přímá kontrola u klienta) |
| 400 | Already exists. | REMOTE-PATH ukazuje na standardní adresář/soubor, který již existuje a je použita operace mkd či put |
| 400 | Not a file. | REMOTE-PATH neukazuje na standardní soubor, ale je použita operace del, get (operace nad soubory) |
| 400 | Not a directory. | REMOTE-PATH ukazuje na soubor, ale je použita operace lst, rmd (operace nad složkami) |
| 400 | Directory not empty. | REMOTE-PATH ukazuje na adresář, který není prázdný a je použita operace rmdir |
| 400 | Unknown error. | Došlo k jiné, než výše zmíněné chybě, není známa přesná příčina problému, nebo došlo k operaci, která nebyla externě povolena, např. mazání uživatelů |


**30.3.2017**

