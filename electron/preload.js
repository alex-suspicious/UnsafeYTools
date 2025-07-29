const { contextBridge, webUtils, ipcRenderer } = require('electron')


contextBridge.exposeInMainWorld('electron', {
  getPathForFile: (file) => {
    return webUtils.getPathForFile(file);
  },
  encode: (filePath, newFile, token, index) => ipcRenderer.invoke('encode', filePath, newFile, token, index),
  ffmpeg: () => ipcRenderer.invoke('ffmpeg').then((result) => {
    console.log(`FFmpeg path: ${result}`);
    return result;
  })
})

ipcRenderer.on('update-file-status', (event, message) => {

    if (!isNaN(message.percent)) {
        console.log(message);

        console.log(`File status updated: ${message.percent}`);
        document.getElementById(`file-processing-width-${message.index}`).style.width = `${Math.floor(message.percent)}%`;
        document.getElementById(`file-processing-percent-${message.index}`).textContent = `${Math.floor(message.percent)}%`;
    }else if( message.percent.includes("Finished") ){
        console.log(`File processing finished: ${message.percent}`);
        document.getElementById(`file-processing-width-${message.index}`).style.width = `100%`;
        document.getElementById(`file-processing-percent-${message.index}`).textContent = `100%`;
        setTimeout(function(){
            document.getElementById(`file-processing-percent-${message.index}`).remove();
            document.getElementById(`file-processing-progressbar-${message.index}`).remove();

            var audio = document.getElementById('success_player');
            audio.play();
        },1500);
    }
});

