const { app, BrowserWindow, ipcMain } = require('electron/main')
const path = require('node:path')
const { spawn } = require('child_process');
const { Menu } = require('electron');

ipcMain.on('ondragstart', (event, filePath) => {
event.sender.startDrag({
    file: path.join(__dirname, filePath),
    icon: iconName
})
})

var win;

const createWindow = () => {
    win = new BrowserWindow({
        width: 800,
        height: 600,
        webPreferences: {
            preload: path.join(__dirname, 'preload.js'),
        },
    })
    
    if (process.platform === "win32" || process.platform === "linux") {
        win.removeMenu();
    } else if (process.platform === "darwin") {
        Menu.setApplicationMenu(Menu.buildFromTemplate([]));
    }

    if( !app.isPackaged )
        win.webContents.openDevTools();

    win.loadFile('index.html');

    win.webContents.on('new-window', function(e, url) {
        e.preventDefault();
        require('electron').shell.openExternal(url);
    });
}

app.whenReady().then(() => {
    ipcMain.handle('encode', (trash, filePath, newFile, token, index) => {
        var processorName = "video_processor";

        if (process.platform === "win32") 
            processorName = "video_processor.exe";

        const binaryPath = app.isPackaged
            ? path.join(process.resourcesPath, processorName)
            : path.join(__dirname, processorName);

        console.log(`${binaryPath} "${filePath}" "${newFile}" "${token}"`);

        const child = spawn(binaryPath, [filePath, newFile, token]);

        child.stdout.on('data', (data) => {
          // 'data' will contain a Buffer, convert it to a string
          console.log(`Child process stdout: ${data.toString()}`);
          win.webContents.send('update-file-status', {percent: data.toString(), index: index});
          // You can send this data to your Electron renderer process using IPC
        });
        
        child.stderr.on('data', (data) => {
          console.error(`Child process stderr: ${data.toString()}`);
        });
        
        child.on('close', (code) => {
          console.log(`Child process exited with code ${code}`);
        });
        
        child.on('error', (err) => {
          console.error('Failed to start child process:', err);
        });
    })

    ipcMain.handle('ffmpeg', () => {
        return new Promise((resolve, reject) => {
            const ffmpegPath = 'ffmpeg';
            const child = spawn(ffmpegPath, ['-version']);

            child.on('error', (err) => {
                console.error('Failed to start ffmpeg:', err);
                reject(false);
            });

            child.stdout.on('data', (data) => {
                console.log(`ffmpeg stdout: ${data}`);
                resolve(true);
            });

            child.stderr.on('data', (data) => {
                console.error(`ffmpeg stderr: ${data}`);
            });
        });
    })

    createWindow()
  
    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) createWindow()
    })
})

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit()
})

