function generateRandomString(length) {
    const randomString = Math.random().toString(36).substring(2, 2 + length);
    return randomString;
}

document.addEventListener('drop', async (event) => {
    event.preventDefault(); // Prevent default behavior (e.g., opening file in new window)
    const droppedFiles = Array.from(event.dataTransfer.files);

    for (const file of droppedFiles) {
        var token = generateRandomString(20);

        const filePath = await window.electron.getPathForFile(file);
        var newFile = filePath.replace(file.name, file.name.replace(".", "_unsafe."));

        processingFiles.push({
            filePath: filePath,
            newFile: newFile,
            fileName: file.name,
            newFileName: file.name.replace(".", "_unsafe."),
            token: token,
            status: 0
        });

        var index = processingFiles.length - 1;

        await window.electron.encode(filePath, newFile, token, index);

        const fileItem = document.createElement("div");
        fileItem.className = "file-item";
        fileItem.innerHTML = fileItem.innerHTML + `
            <li class="list-group-item pb-3">
                <span>
                    <h1 class="d-inline">${file.name}</h1>
                    <small class="text-body-secondary d-inline ms-2">${token}</small>    
                </span>
                <div class="row">
                    <div class="col-11">
                        <small class="text-body-secondary">${newFile}</small>
                    </div>
                    <div class="col-1 text-end">
                        <small id="file-processing-percent-${index}" class="text-body-secondary">0%</small>
                    </div>
                </div>
                <div id="file-processing-progressbar-${index}" class="progress" role="progressbar" aria-label="Animated striped example" aria-valuemin="0" aria-valuemax="100">
                    <div id="file-processing-width-${index}" class="progress-bar progress-bar-striped progress-bar-animated" style="width: 0%"></div>
                </div>
            </li>
        `;
        document.getElementById("file-list").appendChild(fileItem);
        //updateFilesList();
    }

    document.getElementById("file-taker").style.display = "none";
});

document.addEventListener('dragover', (e) => {
	e.preventDefault();
	e.stopPropagation();
});

document.addEventListener('dragenter', (event) => {
	console.log('File is in the Drop Space');
    document.getElementById("file-taker").style.display = "flex";
});

document.addEventListener('dragleave', (event) => {
	console.log('File has left the Drop Space');
    document.getElementById("file-taker").style.display = "none";
});

window.electron.ffmpeg().then((result) => {
    if( result ){
        document.getElementById("ffmpeg-error").remove();
    }
});
