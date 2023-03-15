(async function () {
        const refreshDeviceBtn = document.getElementById("refreshDevice");
        const setFrameTypeBtn = document.getElementById("setFrameType");
        socket = new WebSocket("wss://10.42.0.1:5000", "network-protocol");


        refreshDeviceBtn.addEventListener("click", () => {
                // socket.send("setFrameType");
                // console.log("setFrameType");
                // Create WebSocket connection
                socket = new WebSocket("wss://10.42.0.1:5000", "test");
                // Listen for possible errors
                socket.addEventListener("error", (event) => {
                        console.log("WebSocket error: ", event);
                });

        });


        setFrameTypeBtn.addEventListener("click", () => {
                socket.send("setFrameType");
                console.log("setFrameType");
        });

        const urlCreator = window.URL || window.webkitURL;
        socket.onmessage = async function (event) {
                const blob = await event.data.blob();
                var imageUrl = urlCreator.createObjectURL(blob);
                const imageViewer = document.getElementById("my-image");
                imageViewer.src = imageUrl;
        };
})();
