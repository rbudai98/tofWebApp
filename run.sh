sudo docker build -t my-apache2 .
sudo docker run -dit --name my-running-app -p 8080:80 my-apache2