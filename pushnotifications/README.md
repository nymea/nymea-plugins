# Push Notifications

This plugin allows to send push notifications to mobile devices.

## Supported platforms

* Android (using Firebase Cloud Messaging)
* iOS (using Firebase Cloud Messaging)
* Ubuntu/UBPorts Phone

## Requirements

A push notification device token is required during setup. For Android *and* iOS, a Firebase
token is required. Note that native iOS push tokens are not supported as the Apple Push Notification
Service (APNs) does not allow sending messages in such a distributed manner, however, Firebase is available
for iOS too. On Ubuntu, the UBPorts push services are used.

## More

### Setup

During setup, the token, the push service system and a client id needs to be provided. The token is normally
obtained by the operating system. The push service should be selected from FB-GCM, FB-APNs or UBPorts. The client id
must a unique per client and as persistent as possible.

> Note: Even when using APNs, the token must be obtained using the Firebase SDK as plain APNs does not support sending push notifications from a distributed setup like nymea, but always requires a centralized server on the internet handling all messages.


As it is impossible for an end user to obtain this token, a client app should prefill all the parameters
when setting up a push notification thing. Normally, push tokens expire after a while (for instance when
the user clears app data, when the operating system decides to cycle tokens etc). In this case, the client
app should reconfigure its own push notification thing, updating the token with the new one. The client ID
parameter schould be used to find the appropriate thing to reconfigure.


### Extra data
When sending a push notification, additional data can be passed which will delivered to the client (e.g. nymea:app).

nymea:app currently supports the following extra data:

* open=view: Open a particular view in the app. This can be a main view or a thing id. 
* execute=action&thingId=<thingId>&actionParams=<params json>

Examples:

Open the Dashboard view:

    open=dashboard

Open a particular thing by its ID:

    open=5634307c-8569-4365-a78a-45e7e97966cb

Turn on a light:

    execute=power&thingId=5634307c-8569-4365-a78a-45e7e97966cb&actionParams="{\"power\":true}"


