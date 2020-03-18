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

During setup, the token, the push service system and a client id needs to be provided. The token is normally
obtained by the operating system. The push service should be selected from GCM, APNs or UBPorts. The client id
must a unique per client and as persistent as possible.

> Note: Even when using APNs, the token must be obtained using the Firebase SDK as plain APNs does not support sending push notifications from a distributed setup like nymea, but always requires a centralized server on the internet handling all messages.


As it is impossible for an end user to obtain this token, a client app should prefill all the parameters
when setting up a push notification thing. Normally, push tokens expire after a while (for instance when
the user clears app data, when the operating system decides to cycle tokens etc). In this case, the client
app should reconfigure its own push notification thing, updating the token with the new one. The client ID
parameter schould be used to find the appropriate thing to reconfigure.
