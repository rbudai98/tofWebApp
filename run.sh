# In case previous container is running needs removeing before starting the new one
sudo docker container rm -f /my-running-app
# Building web server for amr64 in container
sudo docker build -t my-apache2 .
# Starting webserver
sudo docker run -dit --name my-running-app -p 8080:80 my-apache2