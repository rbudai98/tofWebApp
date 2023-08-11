FROM httpd:2.4-alpine
RUN apk add --update npm
RUN npm install google-protobuf
COPY ./tofWebApp/ /usr/local/apache2/htdocs/