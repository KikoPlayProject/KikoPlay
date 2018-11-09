var dp;
var curNode;
function postPlayTime(asyncVal)
{
    if(curNode!=undefined)
    {
        var curTime=dp.video.currentTime,duration=dp.video.duration;
        var state=0;
        if(curTime>15 && curTime<duration-15)
        {
            state=1;
        }
        else if(curTime>duration-15)
        {
            state=2;
        }
        if(state!=0)
        {
            $.ajax({
                type: 'POST',
                async:asyncVal,
                url: '/api/updateTime',
                data: JSON.stringify({mediaId: curNode['mediaId'],playTime:curTime,playTimeState:state}),
                contentType: 'application/json'
            });
        }
    }
}
$(document).ready(function() {
    dp = new DPlayer({
        container: document.getElementById('dplayer'),
        mutex: true,                
        danmaku: {
            api: 'api/danmu/',          
            bottom: '15%',        
        }
    });
    dp.on('ended', function () {
        if(curNode!=undefined)
        {
            $.ajax({
                type: 'POST',
                url: '/api/updateTime',
                data: JSON.stringify({mediaId: curNode['mediaId'],playTime:0,playTimeState:2}),
                contentType: 'application/json'
            });
        }
    });
    $('#playlist').treeview({});
    function updatePlaylist() 
    {
        $.ajax({
            url: '/api/playlist',
            contentType: 'application/json',
            success: function(playlist) {
                $('#playlist').treeview({
                    data: playlist,
                    showBorder:false,
                    levels:1,
                    onNodeSelected: function(event, node) {
                        if(node['nodes']==undefined)
                        {
                            postPlayTime(true);
                            curNode=node;
                            dp.switchVideo({
                                url: 'media/'+node['mediaId']
                            }, {
                                id: node['danmuPool'],
                                api: 'api/danmu/',
                                bottom: '15%'
                            });
                            if(node['playTimeState']==1)
                                dp.seek(node['playTime']);
                            dp.play();
                        }
                    }
                });
            }
        });
    }
    updatePlaylist();
});
window.onbeforeunload = function(){
	postPlayTime(false);
}; 
