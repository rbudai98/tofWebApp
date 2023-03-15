(async function () {
        const refreshDeviceBtn = document.getElementById("refreshDevice");
        const setFrameTypeBtn = document.getElementById("setFrameType");


        window.addEventListener("load", (event) => {
                // Create WebSocket connection
                socket = new WebSocket("ws://10.42.0.1:5000", "network-protocol");
                // Listen for possible errors
                socket.addEventListener("error", (event) => {
                        console.log("WebSocket error: ", event);
                });

                protobuf.load("resources/buffer.proto");
        });

        refreshDeviceBtn.addEventListener("click", () => {
                // Create WebSocket connection
                socket = new WebSocket("ws://10.42.0.1:5000", "network-protocol");
                // Listen for possible errors
                socket.addEventListener("error", (event) => {
                        console.log("WebSocket error: ", event);
                });
        });


        setFrameTypeBtn.addEventListener("click", () => {
                // socket.send("setFrameType");
                console.log("setFrameType");
        });

        const urlCreator = window.URL || window.webkitURL;
        // socket.onmessage = async function (event) {
        //         const blob = await event.data.blob();
        //         var imageUrl = urlCreator.createObjectURL(blob);
        //         const imageViewer = document.getElementById("my-image");
        //         imageViewer.src = imageUrl;
        // };
})();
