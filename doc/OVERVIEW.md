Overview of SamRewritten's internal logic
===

This is technical documentation. The aim of this document is to provide the basic principles of the inner workings of SamRewritten to make it easier for new contributors to get started.

I (PaulCombal) will be referring to all the contributors of SamRewritten as "we", without whom this project would not have been complete. Thanks to all of you.

## Tools

SamRewritten is written in C++ exclusively. 

It uses [GTKmm](https://gtkmm.org/) for the GUI. The designs have been created with the Gnome tool [Glade](https://glade.gnome.org/). 

When building with `make dev`, SamRewritten's performance is monitored with [Valgrind](https://valgrind.org/), a tool used to profile resource usage. It results in the creation of massif files, which can be opened with different tools.

## Third-party libraries

For the command-line argument parsing, SamRewritten uses [cxxopts](https://github.com/jarro2783/cxxopts).

SamRewritten uses [yajl](https://lloyd.github.io/yajl/) for JSON serialization and deserialization internally.

To interface with Steam, SamRewritten uses Valve's Steamworks C++ SDK, available for download from [this page](https://partner.steamgames.com/doc/sdk). The file `bin/libsteam_api.so` as well as the folder `steam/` originate from this SDK.

For CLI output table formatting, SamRewritten uses [TextTable](https://github.com/haarcuba/cpp-text-table).

## Standard usage of Steamworks SDK

You can find here [the framework's documentation](https://partner.steamgames.com/doc/sdk/api). Please have a quick overview of it to understand the following paragraphs.

Usually, when making a app with the Steamworks SDK, one must call the function [`SteamAPI_Init`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_Init) when booting the app in order to access all the features from the SDK afterwards. This will not only make the SDK usable, but also notify Steam that a Steam app was launched. In order for Steam to know which app has started, the Steam AppID must be set as the `SteamAppId` environment variable before executing the previously mentioned function. 

The SDK is dynamically linked and uses functions from the shared object `libsteam_api.so`.

After testing, it has been found that Steam will only consider the game shut down when the calling process terminates (without leaving zombie processes).

To give a general idea of the purpose of the SDK, it includes functions such as:
* to retrieve and set achievements & stats
* to manipulate scoreboards
* to check the ownership of DLCs and apps
* get your in-game Steam avatar
* etc.

## SamRewritten's way of retrieving owned apps

The Steamworks SDK provides an accessor `SteamClient()` to interface with Steam and use all the functionalities of the framework. However this accessor is not available unless [`SteamAPI_Init`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_Init) has been called. We do not want to call this function yet, as we do not have any app to start. This is a problem.

There is a way around it. Steam comes with a shared object `steamclient.so`, which include Steam's signature for the Steam Client code. We will use it instead. This can be found in [`MySteamClient.h`](/src/controller/MySteamClient.h). This shared object *must* be loaded from the Steam installation directory of the local machine.

Now that we can access an [`ISteamClient`](https://partner.steamgames.com/doc/api/ISteamClient), we need to retrieve the game list of the current user. The only method at our disposal is `ISteamApps::BIsSubscribedApp`. As far as we know, there is no way to retrieve the list of owned apps and games.

That is why we started another project, which is to make a database of all apps on Steam with their AppID and name. The database can be created/updated with [this tool](https://github.com/PaulCombal/SteamAppsList), and the database itself is found [here](https://github.com/PaulCombal/SteamAppsListDumps). This database (which is really just a big JSON file) is seldom updated, feel free to help there too.

What happens is that SamRewritten downloads that list of apps and caches it, then tests for each app one by one if it is owned by the current user by calling `ISteamApps::BIsSubscribedApp`. The retrieval is implemented at [`SteamAppDAO.cpp#L60`](/src/controller/SteamAppDAO.cpp#L60). With this information we can deduce the user's app library.

While writing this documentation, I noticed a new interface [`ISteamAppList`](https://github.com/Facepunch/Facepunch.Steamworks/blob/master/Generator/steam_sdk/isteamapplist.h). Could this help?

## What happens when starting a game?

When starting a game, SamRewritten `fork`s, and the child process will follow the setup functions from the Steamworks SDK to act as if it were the selected game booting up. This is done by setting the environment variable `SteamAppId` and calling [`SteamAPI_Init`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_Init), as every game should do. Basically, a child process is created, and it notifies Steam that an app started, as instructed in the documentation.

This can be observed in [`src/controller/GameServerManager`](/src/controller/GameServerManager.cpp).

Internally, the child process is called server, and the parent process, client (GUI side). They communicate via sockets.

## How is data transmitted between the UI and the server?

Both the client and server send JSON serialized data through a UNIX socket. Like traditional web technologies, only the client side initiates requests.

*This means that (with a bit of hacking) anyone can build his own frontend for SamRewritten.*

When building SamRewritten in `dev` mode, you can see in plain text the messages being exchanged in the console.

The (de)serialization implementation is found under [`src/json`](/src/json).

The socket implementation is found under [`src/sockets`](/src/sockets). The file names are explicit, except for for [`MyGameSocket`](/src/sockets/MyGameSocket.cpp), which also includes request processing, the "backend" logic behind SamRewritten.

## How does the server process retrieve achievements and stats data?

As mentioned above, [`MyGameSocket`](/src/sockets/MyGameSocket.cpp) is responsible for server logic. It implements much of the Steamworks SDK features, so to understand it fully, we encourage you to familiarize yourself with [the framework's documentation](https://partner.steamgames.com/doc/sdk/api).

If you spend more time looking at the implementation, you can see that something odd happening. Indeed, part of the achievement data is retrieved through the Steamworks API (legacy code), and the rest (as well as the stats) are retrieved through a ["schema parser"](/src/schema_parser). Indeed, some information about the achievements cannot be retrieved through Steamworks, like the achievement icon URL.

That is why we implemented a parser, that will explore Steam's internal cache files to retrieve additional information. Note that the relevant cache isn't always loaded until the app is started: we did not find a way around calling [`SteamAPI_Init`](https://partner.steamgames.com/doc/sdk/api#SteamAPI_Init) with reliable results. 

This leaves us with two means of retrieving data: schema parsing and Steamworks API calls. This can be confusing. That is why in the future, we plan to remove API calls as much as possible and use schema parsing as much as possible, since this method allows the retrieval of all required data.

## How are stats and achievements updated?

Stats and achievements are updated as shown in the Steamworks SDK. The full process can be summed up as emulating the app, and using this emulated app to unlock achievements as the real app would.

The actual implementation can be found at [`MyGameSocket#L289`](/src/sockets/MyGameSocket.cpp#L289).

We can always wonder, is this bannable or detectable? Maybe, in any case we strongly advise avoiding VAC-enabled games, and as said in the README, the contributors are not responsible for any consequences on your usage of SamRewritten. It was written for fun, and educational purposes only (believe it or not).

## What's next?

I'm an old man now, I barely do gaming or programming anymore. Maybe I will again someday. In the meantime, please continue asking questions and opening issues. We will do our best to answer you and get us excited to contribute again.

You will notice, in the ["Projects"](https://github.com/PaulCombal/SamRewritten/projects/1) tab, a kanban with a few tasks. These are the next steps to make SamRewritten cleaner and better. If there are any priorities on this project, you can find them there.

## How can I contribute?

Please fork the project, commit the changes on your fork, then make a pull request to the `dev` branch. Please provide a description of your changes and why you did them. I can be stupid, so I may ask you questions about your code first, do not always expect to get things merged immediately. We will review your changes when we can!

## That's all?

Is there something that needs to be clarified? Please tell us!

Otherwise, happy coding!
