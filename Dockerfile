FROM httpd:2.4-alpine

# Copy protobus file
COPY ./resources/ /usr/local/apache2/htdocs/resources/
COPY ./tofWebApp/ /usr/local/apache2/htdocs/