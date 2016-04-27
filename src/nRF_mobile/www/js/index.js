var Q = require('q');
var qbluetoothle = require('./qbluetoothle');
var Badge = require('./badge');

var badges = [];

var watchdogTimer = null;

var LOCALSTORAGE_GROUP_KEY = "groupkey";
var APP_KEY = "$*)J#(KD#()#Fj80qf80jq4f*)JFQ$";
var BASE_URL = "http://18.111.23.176:8000/";

/***********************************************************************
 * Model Declarations
 */

function Group(groupJson) {
    this.name = groupJson.name;
    this.key = groupJson.key;
    this.show_widget = groupJson.show_widget;
    this.members = [];
    for (var i = 0; i < groupJson.members.length; i++) {
        this.members.push(new GroupMember(groupJson.members[i]));
    }
}

function GroupMember(memberJson) {
    this.name = memberJson.name;
    this.key = memberJson.key;
    this.badge = memberJson.badge;
}

function Meeting(members, type, moderator, description) {
    this.members = members;
    this.type = type;
    this.moderator = moderator;
    this.description = description;
}

PAGES = [];

function Page(id, onInit, onShow, onHide, extras) {
    PAGES.push(this);
    if (extras) {
        for (var key in extras) {
            this[key] = extras[key];
            if (typeof this[key] === "function") {
                this[key] = this[key].bind(this);
            }
        }
    }

    this.id = id;
    this.onInit = onInit.bind(this);
    this.onShow = onShow.bind(this);
    this.onHide = onHide.bind(this);
}


/***********************************************************************
 * Page Configurations
 */


/**
 * Main Page that displays the list of present users for the group
 * @type {Page}
 */
mainPage = new Page("main",
    function onInit() {
        this.presentBadges = [];
        $("#settings-button").click(function() {
            app.showPage(settingsPage);
        });
        $("#startMeetingButton").click(function() {
            if (false && app.presentBadges.length < 2) {
                navigator.notification.alert("Need at least 2 people present to start a meeting.");
                return;
            }
            app.showPage(meetingConfigPage);
        });
        $("#retryDeviceList").click(function() {
            app.refreshGroupData();
        });
    },
    function onShow() {
        app.badgeScanIntervalID = setInterval(function() {
            app.scanForBadges();
        }, 5000);
        app.scanForBadges();
    },
    function onHide() {
        clearInterval(app.badgeScanIntervalID);
    },
    {
        beginRefreshData: function() {
            $("#devicelistError").addClass("hidden");
            $("#devicelistContainer").addClass("hidden");
            $("#devicelistLoader").removeClass("hidden");
        },
        createGroupUserList: function() {
            $("#devicelistLoader").addClass("hidden");

            if (app.group == null) {
                $("#devicelistError").removeClass("hidden");
                return;
            }

            $("#devicelistError").addClass("hidden");
            $("#devicelistContainer").removeClass("hidden");

            $("#devicelist").empty();
            if (! app.group || ! app.group.members) {
                return;
            }
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                $("#devicelist").append($("<li class=\"item\" data-name='{name}' data-device='{badge}' data-key='{key}'><span class='name'>{name}</span><i class='icon ion-battery-full battery-icon' /><i class='icon ion-happy present-icon' /></li>".format(member)));
            }

            app.scanForBadges();
        },
        displayActiveBadges: function() {

            $("#devicelist .item").removeClass("active");
            for (var i = 0; i < app.group.members.length; i++) {
                var member = app.group.members[i];
                if (member.active) {
                    $("#devicelist .item[data-device='" + member.badge + "']").addClass("active");
                }
            }
        },

    }
);


/**
 * Settings Page that lets you save settings
 * @type {Page}
 */
settingsPage = new Page("settings",
    function onInit() {
        $("#saveButton").click(function() {
            localStorage.setItem(LOCALSTORAGE_GROUP_KEY, $("#groupIdField").val());
            app.showPage(mainPage);
            app.refreshGroupData(function (success) {
                toastr.success("Settings Saved!");
            });
        });
    },
    function onShow() {
        var groupId = localStorage.getItem(LOCALSTORAGE_GROUP_KEY);
        $("#groupIdField").val(groupId);
    },
    function onHide() {
    }
);


/**
 * Meeting Config Page that sets up the meeting before it starts
 * @type {Page}
 */
meetingConfigPage = new Page("meetingConfig",
    function onInit() {
        $("#startMeetingConfirmButton").click(function() {
            var type = $("#meetingTypeField").val();
            var moderator = $("#mediatorField option:selected").data("key");
            var description = $("#meetingDescriptionField").val();
            app.meeting = new Meeting(this.meetingMembers, type, moderator, description);
            app.showPage(meetingPage);
        });
    },
    function onShow() {
        this.meetingMembers = [];
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            if (member.active) {
                this.meetingMembers.push(member);
            }
        }
        var names = [];
        $("#mediatorField").empty();
        for (var i = 0; i < this.meetingMembers.length; i++) {
            var member = this.meetingMembers[i];
            names.push(member.name);
            $("#mediatorField").append($("<option data-key='" + member.key + "'>" + member.name + "</option>"));
        }
        $("#memberNameList").text(names.join(", "));
    },
    function onHide() {
    }
);

/**
 * Meeting Page that records data from badges for all members
 * @type {Page}
 */
meetingPage = new Page("meeting",
    function onInit() {
        var $this = this;
        $("#endMeetingButton").click(function() {
            navigator.notification.confirm("Are you sure?", function() {
                $this.onMeetingComplete();
            });
        });
    },
    function onShow() {
        window.plugins.insomnia.keepAwake();
    },
    function onHide() {
        window.plugins.insomnia.allowSleepAgain();
    },
    {
        onMeetingComplete: function() {
            app.showPage(mainPage);
        }
    }
);




/***********************************************************************
 * App Navigation Behavior Configurations
 */


app = {
    /**
     * Initializations
     */
    initialize: function() {
        this.initBluetooth();

        document.addEventListener("backbutton", function(e) {
            e.preventDefault();

            if (mainPage.active) {
                navigator.app.exitApp();
            } else {
                app.showPage(mainPage);
            }
        }, false);

        $(".back-button").click(function() {
            app.showPage(mainPage);
        });

        for (var i = 0; i < PAGES.length; i++) {
            PAGES[i].onInit();
        }

        this.refreshGroupData();
        this.showPage(mainPage);
    },
    initBluetooth: function() {

        bluetoothle.initialize(
            app.bluetoothInitializeSuccess,
            {request: true,statusReceiver: true}
        );
    },
    bluetoothInitializeSuccess: function (obj) {
        console.log('Success');

        // Android v6.0 required requestPermissions. If it's Android < 5.0 there'll
        // be an error, but don't worry about it.
        if (cordova.platformId === 'android') {
            console.log('Asking for permissions');
            bluetoothle.requestPermission(
                function(obj) {
                    console.log('permissions ok');
                },
                function(obj) {
                    console.log('permissions err');
                }
            );
        }
    },


    /**
     * Functions to refresh the group data from the backend
     */
    refreshGroupData: function() {
        app.onrefreshGroupDataStart();
        var groupId = localStorage.getItem(LOCALSTORAGE_GROUP_KEY);

        $.ajax(BASE_URL + "get_group/" + groupId + "/", {
            dataType:"json",
            success: function(result) {
                if (result.success) {
                    app.group = new Group(result.group);
                } else {
                    app.group = null;
                }
                app.onrefreshGroupDataComplete();
            },
            error: function() {
                app.group = null;
                app.onrefreshGroupDataComplete();
            }
        });

    },
    onrefreshGroupDataStart: function() {
        app.refreshingGroupData = true;
        mainPage.beginRefreshData();
    },
    onrefreshGroupDataComplete: function() {
        app.refreshingGroupData = false;
        mainPage.createGroupUserList();
    },

    /**
     * Functions to get which badges are present
     */
    scanForBadges: function() {
        if (app.scanning || app.refreshingGroupData) {
            return;
        }
        app.scanning = true;
        var activeBadges = [];
        qbluetoothle.stopScan().then(function() {
            qbluetoothle.startScan().then(
                function(obj){ // success
                    console.log("Scan completed successfully - "+obj.status)
                    app.onScanComplete(activeBadges);
                }, function(obj) { // error
                    console.log("Scan Start error: " + obj.error + " - " + obj.message)
                    app.onScanComplete(activeBadges);
                }, function(obj) { // progress
                    activeBadges.push(obj.address);
                });
        });
    },
    onScanComplete: function(activeBadges) {
        app.scanning = false;
        if (! app.group) {
            return;
        }
        for (var i = 0; i < app.group.members.length; i++) {
            var member = app.group.members[i];
            member.active = ~activeBadges.indexOf(member.badge);
        }
        mainPage.displayActiveBadges();
    },


    /**
     * Navigation
     */
    showPage: function(page) {
        if (app.activePage) {
            app.activePage.onHide();
        }
        app.activePage = page;
        $(".page").removeClass("active");
        $("#" + page.id).addClass("active");
        page.onShow();
    },


    /**
     * Legacy
     */
    connectButtonPressed1: function() {
        var badge = badges[0];
        console.log("will try to connect - "+badge.address);
        app.connectButton(badge);
    },
    connectButtonPressed2: function() {
        var badge = badges[1];
        console.log("will try to connect - "+badge.address);
        app.connectButton(badge);
    },    
    connectButton: function(badge) {
        badge.connectDialog();
    },
    discoverButtonPressed:function() {
        var badge = badges[0];
        badge.discover();
    },
    subscribeButtonPressed:function() {
        var badge = badges[0];
        badge.subscribe();
    },
    
    disconnect1ButtonPressed: function() {
        var badge = badges[0];
        badge.close();
    },
    disconnect2ButtonPressed: function() {
        var badge = badges[1];
        badge.close();
    },
    connectToDeviceWrap : function(badge){
        return function() {
            badge.connect();
        }
    },
    closeDeviceWrap : function(badge){
        return function() {
            badge.close();
        }
    },    
    watchdogStart: function() {
        console.log("Starting watchdog");
        watchdogTimer = setInterval(function(){ app.watchdog() }, 1000);  
    },
    watchdogEnd: function() {
        console.log("Ending watchdog");
        if (watchdogTimer != null)
        {
            clearInterval(watchdogTimer);
        }
    },
    sendButtonPressed: function() {
        var badge = badges[0];
        var s = "s"; //status

        badge.sendString(s);
    },
    stateButtonPressed: function() {
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badge.lastActivity;
            var disconnectDatetime = badge.lastDisconnect;
            console.log(badge.address+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);
        }
    },
    watchdog: function() {
        for (var i = 0; i < badges.length; ++i) {
            var badge = badges[i];
            var activityDatetime = badge.lastActivity;
            var disconnectDatetime = badge.lastDisconnect;
            console.log(badge.address+"|lastActivity: "+activityDatetime+"|lastDisconnect: "+disconnectDatetime);

            var nowDatetimeMs = Date.now();
            var activityDatetimeMs = activityDatetime.getTime();
            var disconnectDatetimeMs = disconnectDatetime.getTime();

            // kill if connecting for too long and/or if no activity (can update when data recieved)
            // call close() just in case
            // next watchdog call should perform connection
            if (nowDatetimeMs - activityDatetimeMs > 15000) {
                console.log(badge.address+"|Should timeout");

                // touch activity and disconnect date to make sure we don't keep calling this
                // and that we are not calling the next function while there's a disconnect hapenning already
                badge.touchLastActivity();
                badge.touchLastDisconnect();

                // close
                var cf = app.closeDeviceWrap(badge);
                cf();

            } else if (nowDatetimeMs - disconnectDatetimeMs > 5000 && disconnectDatetimeMs >= activityDatetimeMs) {
                    // if more than XX seconds since last disconnect 
                    // and disconnect occoured AFTER connect (meanning that there isn't an open session)
                    // call connect
                console.log(badge.address+"|Last disconnected XX seconds ago. Should try to connect again");

                // touch activity date so we know not to try and connect again
                //badge.touchLastActivity();

                // connect
                var cf = app.connectToDeviceWrap(badge);
                cf();
            } else {
                console.log(badge.address+"|Watchdog, Do nothing");
            }
        }
    },
};



/***********************************************************************
 * Initialization of various libraries and global prototype overloads
 */


toastr.options = {
    "closeButton": false,
    "positionClass": "toast-bottom-center",
    "preventDuplicates": true,
    "showDuration": "200",
    "hideDuration": "500",
    "timeOut": "1000",
}

if (!String.prototype.format) {
    String.prototype.format = function() {
        var str = this.toString();
        if (!arguments.length)
            return str;
        var args = typeof arguments[0],
            args = (("string" == args || "number" == args) ? arguments : arguments[0]);
        for (arg in args)
            str = str.replace(RegExp("\\{" + arg + "\\}", "gi"), args[arg]);
        return str;
    }
}

$.ajaxSetup({
    beforeSend: function(xhr, settings) {
        xhr.setRequestHeader("X-APPKEY", APP_KEY);
    }
});

document.addEventListener('deviceready', function() {app.initialize() }, false);
