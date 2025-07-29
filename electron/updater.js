
function updateFilesList(){
    document.getElementById("file-list").innerHTML = "";
    processingFiles.forEach((file, index) => {
        const fileItem = document.createElement("div");
        fileItem.className = "file-item";
        fileItem.innerHTML = `
            <li class="list-group-item pb-3">
                <span>
                    <h1 class="d-inline">${file.fileName}</h1>
                    <small class="text-body-secondary d-inline ms-2">${file.token}</small>    
                </span>
                <div class="row">
                    <div class="col-11">
                        <small class="text-body-secondary">${file.newFile}</small>
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
    });
}