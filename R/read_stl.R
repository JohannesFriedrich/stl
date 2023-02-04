#' Read an STL file
#' @param path [character] (**required**): Path to a stored stl file
#' @param verbose [logical] (**required**): Provide further information about the parsed file
#' @return A matrix
#' @md
#' @export
read_stl <- function(path, verbose = FALSE){

  .Call(readSTL_, path, verbose)

}

