#!/bin/bash

database="address-book-database.csv"

view() {
    column -t -s ',' $database | head -n 1 && tail -n +2 $database | sort -t ',' -k 4 | column -t -s ','

    return 0
}
search() {
    if [ ${#1} -lt 4 ]; then
        echo "La stringa di ricerca è troppo corta."
        return
    fi
    IFS=$'\n'
    found=false
    for entry in $(tail -n +2 $database); do
        if echo "$entry" | grep -q "$1"; then
            echo #empty line (separator)
            IFS=',' read -r name surname phone mail city address <<< "$entry"
            echo "Name: $name"
            echo "Surname: $surname"
            echo "Phone: $phone"
            echo "Mail: $mail"
            echo "City: $city"
            echo "Address: $address"
            found=true
        fi
    done
    if ! $found; then
        echo "Not found"
    fi

    return 0
}

insert() {
    read -p "Name: " name
    read -p "Surname: " surname

    #validazione del numero di telefono, altrimenti lo richiede all'esaurimento se non è numerico
    while true; do
        read -p "Phone: " phone
        if [[ ! $phone =~ ^[0-9]+$ ]]; then
            echo "Inserisci un numero di telefono valido (solo cifre numeriche)."
        else
            break
        fi
    done

    while true; do
        read -p "Mail: " mail

        #controllo mail per vedere se è già presente nel database
        if grep -q "^.*,${mail},.*$" $database; then
            echo "Mail already exists in the address book."
        elif [[ ! ${mail} =~ .*@.* ]]; then
            echo "Indirizzo email non valido (non ha la '@')"
        else 
            break
        fi
    done

    read -p "City: " city
    read -p "Address: " address

    #aggiungiamo i dati al db
    echo "$name,$surname,$phone,$mail,$city,$address" >> $database
    echo "Added"

    return 0
}

delete() {
    local temp_file=$(mktemp)  # Creiamo un file temporaneo per scrivere la nuova versione del file CSV
    local found=false

    while IFS=, read -r name surname phone mail city address; do
        if [ "$mail" != "$1" ]; then
            echo "$name,$surname,$phone,$mail,$city,$address" >> "$temp_file"
        else
            found=true
        fi
    done < "$database"

    if $found; then
        mv "$temp_file" "$database"  # Sostituiamo il file originale con quello aggiornato
        echo "Deleted"
    else
        echo "Cannot find any record."
        rm "$temp_file"  # Eliminiamo il file temporaneo se non è stato utilizzato
    fi

    return 0
}


case $1 in
    "view")
        view
        ;;
    "search")
        search "$2"
        ;;
    "insert")
        insert
        ;;
    "delete")
        delete "$2"
        ;;
    *)
        echo "Usage: $0 {view|search <string>|insert|delete <mail>}"
        exit 1
        ;;
esac
