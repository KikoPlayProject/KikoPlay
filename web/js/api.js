const KikoAPI = {
    async getPlaylist() {
        const res = await fetch('/api/playlist');
        return res.json();
    },

    async getRecent() {
        const res = await fetch('/api/recent');
        return res.json();
    },

    async updateTime(mediaId, playTime, playTimeState) {
        return fetch('/api/updateTime', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ mediaId, playTime, playTimeState })
        });
    },

    updateTimeSync(mediaId, playTime, playTimeState) {
        navigator.sendBeacon(
            '/api/updateTime',
            new Blob(
                [JSON.stringify({ mediaId, playTime, playTimeState })],
                { type: 'application/json' }
            )
        );
    },

    async getSubtitle(mediaId) {
        const res = await fetch('/api/subtitle?id=' + encodeURIComponent(mediaId));
        return res.json();
    },

    async getDanmakuFull(poolId, update) {
        var url = '/api/danmu/full/?id=' + encodeURIComponent(poolId);
        if (update) url += '&update=true';
        const res = await fetch(url);
        return res.json();
    },

    getMediaUrl(mediaId) {
        return 'media/' + mediaId;
    },

    getSubtitleUrl(format, mediaId) {
        return 'sub/' + format + '/' + mediaId;
    },

    getDanmakuApi() {
        return 'api/danmu/';
    }
};
