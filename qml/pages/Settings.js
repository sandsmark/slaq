.pragma library

var userInfo = null;

function setUserInfo(userId, teamId, teamName) {
    userInfo = {
        userId: userId,
        teamId: teamId,
        teamName: teamName
    }
}

function getUserInfo() {
    return userInfo;
}

function hasUserInfo() {
    console.log('hasUserInfo', userInfo !== null)
    return userInfo !== null
}
